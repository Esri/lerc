#include <iostream>
#include <fstream> 
#include <math.h>
#include "BitMask.h"
#include "Lerc.h"
#include <Magick++.h>

int main(int argc, char **argv){
	Magick::InitializeMagick(*argv);
	Magick::Image img;

	double maxZErrorWanted = 0.01;
	double eps = 0.00001;    // safety margin (optional), to account for finite floating point accuracy
	double maxZError = maxZErrorWanted-eps;

	std::string path = "Bliss.bmp";
	img.read(path);

	LercNS::Lerc lerc;
	int w = img.columns();
	int h = img.rows();
	float* floatImg = new float[w * h * 3];
	memset(floatImg, 0, w * h * 3);

	std::cout << "Reading in Image using ImageMagick... (This could take a while) " << std::endl;

	float* arr0 = floatImg + 0 * w * h;
	float* arr9 = floatImg + 1 * w * h;
	float* arr8 = floatImg + 2 * w * h;
	for (int k = 0, i = 0; i < h; i++){
		for (int j = 0; j < w; j++, k++){
			arr0[k] = Magick::ColorRGB(img.pixelColor(j, i)).red();
			arr9[k] = Magick::ColorRGB(img.pixelColor(j, i)).green();
			arr8[k] = Magick::ColorRGB(img.pixelColor(j, i)).blue();
		}
	}

	size_t numBytesNeeded = 0;
	std::cout << "Finished Reading Image using ImageMagick, starting Lerc buffer size Calculation." << std::endl;
	if (!lerc.ComputeBufferSize((void*)floatImg, LercNS::Lerc::DT_Float, w, h, 3, 0, maxZError, numBytesNeeded))
		std::cout << "ComputeBufferSize failed" << std::endl;


	size_t numBytesBlob = numBytesNeeded;
	size_t numBytesWritten = 0;
	Byte* pLercBlob = new Byte[numBytesBlob];
	std::cout << "Start to encode image with lerc." << std::endl;
	if (!lerc.Encode((void*)floatImg,    // raw image data, row by row, band by band
		LercNS::Lerc::DT_Float,
		w, h, 3,
		0,                   // 0 if all pixels are valid
		maxZError,           // max coding error per pixel, or precision
		pLercBlob,           // buffer to write to, function will fail if buffer too small
		numBytesBlob,        // buffer size
		numBytesWritten))    // num bytes written to buffer
	{
		std::cout << "Encode failed" << std::endl;
	}

	std::cout << "Encode Finished, total size: "<< numBytesBlob <<" Bytes, writing encoded file to disk"<< std::endl;
	//write blob to disk
	FILE* file;
	fopen_s(&file, "out.lerc2", "w");
	fwrite(pLercBlob, 1, numBytesBlob, file);


	// new data storage
	float* floatImg3 = new float[w * h * 3];
	memset(floatImg3, 0, w * h * 3 * sizeof(float));

	LercNS::BitMask bitMask3(w, h);
	bitMask3.SetAllValid();
	LercNS::Lerc::LercInfo lercInfo;
	std::cout << "Starting to decode with Lerc." << std::endl;
	// decompress
	if (!lerc.GetLercInfo(pLercBlob, numBytesBlob, lercInfo))
		std::cout << "get header info failed" << std::endl;

	if (lercInfo.nCols != w || lercInfo.nRows != h || lercInfo.nBands != 3 || lercInfo.dt != LercNS::Lerc::DT_Float)
		std::cout << "got wrong lerc info" << std::endl;

	if (!lerc.Decode(pLercBlob, numBytesBlob, &bitMask3, w, h, 3, LercNS::Lerc::DT_Float, (void*)floatImg3))
		std::cout << "decode failed" << std::endl;


	std::cout << "Decoding finished with Lerc." << std::endl << "Writing decoded image to disk using ImageMagick." << std::endl;
	Magick::Image outimg;
	Magick::Geometry geom(w, h);
	outimg.size(geom);

	outimg.type(Magick::TrueColorType); 
	outimg.modifyImage(); 
	Magick::Pixels view(outimg);

	Magick::PixelPacket *pointer = view.get(0, 0, w, h);
	float* arr2 = floatImg3 + 0 * w * h;
	float* arr3 = floatImg3 + 1 * w * h;
	float* arr4 = floatImg3 + 2 * w * h;
		
	for (int k = 0, i = 0; i < h; i++){
		for (int j = 0; j < w; j++, k++){
				Magick::ColorRGB color(arr2[k], arr3[k], arr4[k]);
				*pointer++ = color;
		}
		view.sync();
	}
	outimg.write("out.bmp");

	std::cout << "Press Enter to Exit";
	std::cin.get();
}