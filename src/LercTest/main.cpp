
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <vector>
#include <iostream>
#include <chrono>
#include "../LercLib/Lerc_c_api.h"
//#include "../../include/Lerc_c_api.h"
//#include "../../include/Lerc_types.h"    // see for error codes, data types, etc

using namespace std;
using namespace std::chrono;

//#define TestLegacyData

typedef unsigned char Byte;    // convenience
typedef unsigned int uint32;

//-----------------------------------------------------------------------------

enum lerc_DataType { dt_char = 0, dt_uchar, dt_short, dt_ushort, dt_int, dt_uint, dt_float, dt_double };

void BlobInfo_Print(const uint32* infoArr)
{
  const uint32* ia = infoArr;
  printf("version = %d, dataType = %d, nDim = %d, nCols = %d, nRows = %d, nBands = %d, nValidPixels = %d, blobSize = %d\n",
    ia[0], ia[1], ia[2], ia[3], ia[4], ia[5], ia[6], ia[7]);
}

bool BlobInfo_Equal(const uint32* infoArr, uint32 nDim, uint32 nCols, uint32 nRows, uint32 nBands, uint32 dataType)
{
  const uint32* ia = infoArr;
  return ia[1] == dataType && ia[2] == nDim && ia[3] == nCols && ia[4] == nRows && ia[5] == nBands;
}

//-----------------------------------------------------------------------------

int main(int argc, char* arcv[])
{
  lerc_status hr(0);

  // Sample 1: float image, 1 band, with some pixels set to invalid / void, maxZError = 0.1

  int h = 512;
  int w = 512;

  float* zImg = new float[w * h];
  memset(zImg, 0, w * h * sizeof(float));

  Byte* maskByteImg = new Byte[w * h];
  memset(maskByteImg, 0, w * h);

  for (int k = 0, i = 0; i < h; i++)
  {
    for (int j = 0; j < w; j++, k++)
    {
      zImg[k] = sqrt((float)(i * i + j * j));    // smooth surface
      zImg[k] += rand() % 20;    // add some small amplitude noise
      //zImg[k] = NAN;

      if (j % 100 == 0 || i % 100 == 0)    // set some void points
        maskByteImg[k] = 0;
      else
        maskByteImg[k] = 1;
    }
  }


  // compress into byte arr

  double maxZErrorWanted = 0.1;
  double eps = 0.0001;    // safety margin (optional), to account for finite floating point accuracy
  double maxZError = maxZErrorWanted - eps;

  uint32 numBytesNeeded = 0;
  uint32 numBytesWritten = 0;

  hr = lerc_computeCompressedSize((void*)zImg,    // raw image data, row by row, band by band
    (uint32)dt_float, 1, w, h, 1,
    maskByteImg,         // can give nullptr if all pixels are valid
    maxZError,           // max coding error per pixel, or precision
    &numBytesNeeded);    // size of outgoing Lerc blob

  if (hr)
    cout << "lerc_computeCompressedSize(...) failed" << endl;

  uint32 numBytesBlob = numBytesNeeded;
  Byte* pLercBlob = new Byte[numBytesBlob];

  high_resolution_clock::time_point t0 = high_resolution_clock::now();

  hr = lerc_encode((void*)zImg,    // raw image data, row by row, band by band
    (uint32)dt_float, 1, w, h, 1,
    maskByteImg,         // can give nullptr if all pixels are valid
    maxZError,           // max coding error per pixel, or precision
    pLercBlob,           // buffer to write to, function will fail if buffer too small
    numBytesBlob,        // buffer size
    &numBytesWritten);   // num bytes written to buffer

  if (hr)
    cout << "lerc_encode(...) failed" << endl;

  high_resolution_clock::time_point t1 = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(t1 - t0).count();

  double ratio = w * h * (0.125 + sizeof(float)) / numBytesBlob;
  cout << "sample 1 compression ratio = " << ratio << ", encode time = " << duration << " ms" << endl;


  // decompress

  uint32 infoArr[10];
  double dataRangeArr[3];
  hr = lerc_getBlobInfo(pLercBlob, numBytesBlob, infoArr, dataRangeArr, 10, 3);
  if (hr)
    cout << "lerc_getBlobInfo(...) failed" << endl;

  BlobInfo_Print(infoArr);

  if (!BlobInfo_Equal(infoArr, 1, w, h, 1, (uint32)dt_float))
    cout << "got wrong lerc info" << endl;

  // new empty data storage
  float* zImg3 = new float[w * h];
  memset(zImg3, 0, w * h * sizeof(float));

  Byte* maskByteImg3 = new Byte[w * h];
  memset(maskByteImg3, 0, w * h);

  t0 = high_resolution_clock::now();

  hr = lerc_decode(pLercBlob, numBytesBlob, maskByteImg3, 1, w, h, 1, (uint32)dt_float, (void*)zImg3);
  if (hr)
    cout << "lerc_decode(...) failed" << endl;

  t1 = high_resolution_clock::now();
  duration = duration_cast<milliseconds>(t1 - t0).count();


  // compare to orig

  double maxDelta = 0;
  for (int k = 0, i = 0; i < h; i++)
  {
    for (int j = 0; j < w; j++, k++)
    {
      if (maskByteImg3[k] != maskByteImg[k])
        cout << "Error in main: decoded valid bytes differ from encoded valid bytes" << endl;

      if (maskByteImg3[k])
      {
        double delta = fabs(zImg3[k] - zImg[k]);
        if (delta > maxDelta)
          maxDelta = delta;
      }
    }
  }

  cout << "max z error per pixel = " << maxDelta << ", decode time = " << duration << " ms" << endl;
  cout << endl;

  delete[] zImg;
  delete[] zImg3;
  delete[] maskByteImg;
  delete[] maskByteImg3;
  delete[] pLercBlob;
  pLercBlob = 0;

  //---------------------------------------------------------------------------

  // Sample 2: random byte image, nDim = 3, all pixels valid, maxZError = 0 (lossless)

  h = 713;
  w = 257;

  Byte* byteImg = new Byte[3 * w * h];
  memset(byteImg, 0, 3 * w * h);

  for (int k = 0, i = 0; i < h; i++)
    for (int j = 0; j < w; j++, k++)
      for (int m = 0; m < 3; m++)
        byteImg[k * 3 + m] = rand() % 30;


  // encode

  hr = lerc_computeCompressedSize((void*)byteImg,    // raw image data: nDim values per pixel, row by row, band by band
    (uint32)dt_uchar, 3, w, h, 1,
    0,                   // can give nullptr if all pixels are valid
    0,                   // max coding error per pixel
    &numBytesNeeded);    // size of outgoing Lerc blob

  if (hr)
    cout << "lerc_computeCompressedSize(...) failed" << endl;

  numBytesBlob = numBytesNeeded;
  pLercBlob = new Byte[numBytesBlob];

  t0 = high_resolution_clock::now();

  hr = lerc_encode((void*)byteImg,    // raw image data: nDim values per pixel, row by row, band by band
    (uint32)dt_uchar, 3, w, h, 1,
    0,                   // can give nullptr if all pixels are valid
    0,                   // max coding error per pixel
    pLercBlob,           // buffer to write to, function will fail if buffer too small
    numBytesBlob,        // buffer size
    &numBytesWritten);   // num bytes written to buffer

  if (hr)
    cout << "lerc_encode(...) failed" << endl;

  t1 = high_resolution_clock::now();
  duration = duration_cast<milliseconds>(t1 - t0).count();

  ratio = 3 * w * h / (double)numBytesBlob;
  cout << "sample 2 compression ratio = " << ratio << ", encode time = " << duration << " ms" << endl;


  // decode

  hr = lerc_getBlobInfo(pLercBlob, numBytesBlob, infoArr, dataRangeArr, 10, 3);
  if (hr)
    cout << "lerc_getBlobInfo(...) failed" << endl;

  BlobInfo_Print(infoArr);

  if (!BlobInfo_Equal(infoArr, 3, w, h, 1, (uint32)dt_uchar))
    cout << "got wrong lerc info" << endl;

  // new data storage
  Byte* byteImg3 = new Byte[3 * w * h];
  memset(byteImg3, 0, 3 * w * h);

  t0 = high_resolution_clock::now();

  hr = lerc_decode(pLercBlob, numBytesBlob, 0, 3, w, h, 1, (uint32)dt_uchar, (void*)byteImg3);
  if (hr)
    cout << "lerc_decode(...) failed" << endl;

  t1 = high_resolution_clock::now();
  duration = duration_cast<milliseconds>(t1 - t0).count();

  // compare to orig

  maxDelta = 0;
  for (int k = 0, i = 0; i < h; i++)
    for (int j = 0; j < w; j++, k++)
      for (int m = 0; m < 3; m++)
      {
        double delta = abs(byteImg3[k * 3 + m] - byteImg[k * 3 + m]);
        if (delta > maxDelta)
          maxDelta = delta;
      }

  cout << "max z error per pixel = " << maxDelta << ", decode time = " << duration << " ms" << endl;
  cout << endl;

  delete[] byteImg;
  delete[] byteImg3;
  delete[] pLercBlob;
  pLercBlob = 0;

  //---------------------------------------------------------------------------


#ifdef TestLegacyData

  Byte* pLercBuffer = new Byte[4 * 2048 * 2048];
  Byte* pDstArr     = new Byte[4 * 2048 * 2048];
  Byte* pValidBytes = new Byte[1 * 2048 * 2048];

  vector<string> fnVec;
  string path = "D:/GitHub/LercOpenSource/testData/";

  fnVec.push_back("amazon3.lerc1");
  fnVec.push_back("tuna.lerc1");
  fnVec.push_back("tuna_0_to_1_w1920_h925.lerc1");

  fnVec.push_back("testbytes.lerc2");
  fnVec.push_back("testHuffman_w30_h20_uchar0.lerc2");
  fnVec.push_back("testHuffman_w30_h20_ucharx.lerc2");
  fnVec.push_back("testHuffman_w1922_h1083_uchar.lerc2");

  fnVec.push_back("testall_w30_h20_char.lerc2");
  fnVec.push_back("testall_w30_h20_byte.lerc2");
  fnVec.push_back("testall_w30_h20_short.lerc2");
  fnVec.push_back("testall_w30_h20_ushort.lerc2");
  fnVec.push_back("testall_w30_h20_long.lerc2");
  fnVec.push_back("testall_w30_h20_ulong.lerc2");
  fnVec.push_back("testall_w30_h20_float.lerc2");

  fnVec.push_back("testall_w1922_h1083_char.lerc2");
  fnVec.push_back("testall_w1922_h1083_byte.lerc2");
  fnVec.push_back("testall_w1922_h1083_short.lerc2");
  fnVec.push_back("testall_w1922_h1083_ushort.lerc2");
  fnVec.push_back("testall_w1922_h1083_long.lerc2");
  fnVec.push_back("testall_w1922_h1083_ulong.lerc2");
  fnVec.push_back("testall_w1922_h1083_float.lerc2");

  fnVec.push_back("testuv_w30_h20_char.lerc2");
  fnVec.push_back("testuv_w30_h20_byte.lerc2");
  fnVec.push_back("testuv_w30_h20_short.lerc2");
  fnVec.push_back("testuv_w30_h20_ushort.lerc2");
  fnVec.push_back("testuv_w30_h20_long.lerc2");
  fnVec.push_back("testuv_w30_h20_ulong.lerc2");
  fnVec.push_back("testuv_w30_h20_float.lerc2");

  fnVec.push_back("testuv_w1922_h1083_char.lerc2");
  fnVec.push_back("testuv_w1922_h1083_byte.lerc2");
  fnVec.push_back("testuv_w1922_h1083_short.lerc2");
  fnVec.push_back("testuv_w1922_h1083_ushort.lerc2");
  fnVec.push_back("testuv_w1922_h1083_long.lerc2");
  fnVec.push_back("testuv_w1922_h1083_ulong.lerc2");
  fnVec.push_back("testuv_w1922_h1083_float.lerc2");

  for (size_t n = 0; n < fnVec.size(); n++)
  {
    string fn = path;
    fn += fnVec[n];

    FILE* fp = 0;
    fopen_s(&fp, fn.c_str(), "rb");
    fseek(fp, 0, SEEK_END);
    size_t fileSize = ftell(fp);    // get the file size
    fclose(fp);
    fp = 0;

    fopen_s(&fp, fn.c_str(), "rb");
    fread(pLercBuffer, 1, fileSize, fp);    // read Lerc blob into buffer
    fclose(fp);
    fp = 0;

    hr = lerc_getBlobInfo(pLercBuffer, (uint32)fileSize, infoArr, dataRangeArr, 10, 3);
    if (hr)
      cout << "lerc_getBlobInfo(...) failed" << endl;

    {
      int version = infoArr[0];
      uint32 dt = infoArr[1];
      int nDim = infoArr[2];
      int w = infoArr[3];
      int h = infoArr[4];
      int nBands = infoArr[5];
      double zMin = dataRangeArr[0];
      double zMax = dataRangeArr[1];

      t0 = high_resolution_clock::now();

      std::string resultMsg = "ok";
      if (0 != lerc_decode(pLercBuffer, (uint32)fileSize, pValidBytes, nDim, w, h, nBands, dt, (void*)pDstArr))
        resultMsg = "FAILED";

      t1 = high_resolution_clock::now();
      duration = duration_cast<milliseconds>(t1 - t0).count();

      printf("nDim = %1d, w = %4d, h = %4d, nBands = %1d, dt = %1d, min = %10.4f, max = %16.4f, time = %3lld ms,  %s :  %s\n",
          nDim, w, h, nBands, dt, zMin, zMax, duration, resultMsg.c_str(), fnVec[n].c_str());
    }
  }

  delete[] pLercBuffer;
  delete[] pDstArr;
  delete[] pValidBytes;

#endif

  printf("\npress ENTER\n");
  getchar();

  return 0;
}

//-----------------------------------------------------------------------------
