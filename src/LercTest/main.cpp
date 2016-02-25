
#include <tchar.h>
#include <iostream>
#include "Lerc.h"
#include "PerfTimer.h"

using namespace std;
using namespace LercNS;

//#define TestLegacyData

//-----------------------------------------------------------------------------

int _tmain(int argc, _TCHAR* argv[])
{
  // Sample 1: float image, 1 band, with some pixels set to invalid / void, maxZError = 0.1

  int h = 512;
  int w = 512;

  float* zImg = new float[w * h];
  memset(zImg, 0, w * h * sizeof(float));

  LercNS::BitMask bitMask(w, h);
  bitMask.SetAllValid();

  for (int k = 0, i = 0; i < h; i++)
  {
    for (int j = 0; j < w; j++, k++)
    {
      zImg[k] = sqrt((float)(i * i + j * j));    // smooth surface
      zImg[k] += rand() % 20;    // add some small amplitude noise

      if (j % 100 == 0 || i % 100 == 0)    // set some void points
        bitMask.SetInvalid(k);
    }
  }


  // compress into byte arr

  double maxZErrorWanted = 0.1;
  double eps = 0.0001;    // safety margin (optional), to account for finite floating point accuracy
  double maxZError = maxZErrorWanted - eps;

  size_t numBytesNeeded = 0;
  size_t numBytesWritten = 0;
  Lerc lerc;
  PerfTimer pt;

  if (!lerc.ComputeBufferSize((void*)zImg,    // raw image data, row by row, band by band
    Lerc::DT_Float,
    w, h, 1,
    &bitMask,                  // set 0 if all pixels are valid
    maxZError,                 // max coding error per pixel, or precision
    numBytesNeeded))           // size of outgoing Lerc blob
  {
    cout << "ComputeBufferSize failed" << endl;
  }

  size_t numBytesBlob = numBytesNeeded;
  Byte* pLercBlob = new Byte[numBytesBlob];

  pt.start();

  if (!lerc.Encode((void*)zImg,    // raw image data, row by row, band by band
    Lerc::DT_Float,
    w, h, 1,
    &bitMask,           // 0 if all pixels are valid
    maxZError,           // max coding error per pixel, or precision
    pLercBlob,           // buffer to write to, function will fail if buffer too small
    numBytesBlob,        // buffer size
    numBytesWritten))    // num bytes written to buffer
  {
    cout << "Encode failed" << endl;
  }

  pt.stop();

  double ratio = w * h * (0.125 + sizeof(float)) / numBytesBlob;
  cout << "sample 1 compression ratio = " << ratio << ", encode time = " << pt.ms() << " ms" << endl;

  // new data storage
  float* zImg3 = new float[w * h];
  memset(zImg3, 0, w * h * sizeof(float));

  BitMask bitMask3(w, h);
  bitMask3.SetAllValid();


  // decompress

  Lerc::LercInfo lercInfo;
  if (!lerc.GetLercInfo(pLercBlob, numBytesBlob, lercInfo))
    cout << "get header info failed" << endl;

  if (lercInfo.nCols != w || lercInfo.nRows != h || lercInfo.nBands != 1 || lercInfo.dt != Lerc::DT_Float)
    cout << "got wrong lerc info" << endl;

  pt.start();

  if (!lerc.Decode(pLercBlob, numBytesBlob, &bitMask3, w, h, 1, Lerc::DT_Float, (void*)zImg3))
    cout << "decode failed" << endl;

  pt.stop();


  // compare to orig

  double maxDelta = 0;
  for (int k = 0, i = 0; i < h; i++)
  {
    for (int j = 0; j < w; j++, k++)
    {
      if (bitMask3.IsValid(k) != bitMask.IsValid(k))
        cout << "Error in main: decoded bit mask differs from encoded bit mask" << endl;

      if (bitMask3.IsValid(k))
      {
        double delta = fabs(zImg3[k] - zImg[k]);
        if (delta > maxDelta)
          maxDelta = delta;
      }
    }
  }

  cout << "max z error per pixel = " << maxDelta << ", decode time = " << pt.ms() << " ms" << endl;

  delete[] zImg;
  delete[] zImg3;
  delete[] pLercBlob;
  pLercBlob = 0;

  //---------------------------------------------------------------------------

  // Sample 2: random byte image, 3 bands, all pixels valid, maxZError = 0 (lossless)

  h = 713;
  w = 257;

  Byte* byteImg = new Byte[w * h * 3];
  memset(byteImg, 0, w * h * 3);

  for (int iBand = 0; iBand < 3; iBand++)
  {
    Byte* arr = byteImg + iBand * w * h;
    for (int k = 0, i = 0; i < h; i++)
      for (int j = 0; j < w; j++, k++)
        arr[k] = rand() % 30;
  }

  // encode 

  if (!lerc.ComputeBufferSize((void*)byteImg, Lerc::DT_Byte, w, h, 3, 0, 0, numBytesNeeded))
    cout << "ComputeBufferSize failed" << endl;

  numBytesBlob = numBytesNeeded;
  pLercBlob = new Byte[numBytesBlob];

  pt.start();

  if (!lerc.Encode((void*)byteImg,    // raw image data, row by row, band by band
    Lerc::DT_Byte,
    w, h, 3,
    0,                   // 0 if all pixels are valid
    0,                   // max coding error per pixel, or precision
    pLercBlob,           // buffer to write to, function will fail if buffer too small
    numBytesBlob,        // buffer size
    numBytesWritten))    // num bytes written to buffer
  {
    cout << "Encode failed" << endl;
  }

  pt.stop();

  ratio = w * h * 3 / (double)numBytesBlob;
  cout << "sample 2 compression ratio = " << ratio << ", encode time = " << pt.ms() << " ms" << endl;

  // new data storage
  Byte* byteImg3 = new Byte[w * h * 3];
  memset(byteImg3, 0, w * h * 3);

  // decompress

  if (!lerc.GetLercInfo(pLercBlob, numBytesBlob, lercInfo))
    cout << "get header info failed" << endl;

  if (lercInfo.nCols != w || lercInfo.nRows != h || lercInfo.nBands != 3 || lercInfo.dt != Lerc::DT_Byte)
    cout << "got wrong lerc info" << endl;

  pt.start();

  if (!lerc.Decode(pLercBlob, numBytesBlob, 0, w, h, 3, Lerc::DT_Byte, (void*)byteImg3))
    cout << "decode failed" << endl;

  pt.stop();

  // compare to orig

  maxDelta = 0;
  for (int k = 0, i = 0; i < h; i++)
    for (int j = 0; j < w; j++, k++)
    {
      double delta = abs(byteImg3[k] - byteImg[k]);
      if (delta > maxDelta)
        maxDelta = delta;
    }

  cout << "max z error per pixel = " << maxDelta << ", decode time = " << pt.ms() << " ms" << endl;

  delete[] byteImg;
  delete[] byteImg3;
  delete[] pLercBlob;
  pLercBlob = 0;

  //---------------------------------------------------------------------------


#ifdef TestLegacyData

  Byte* pLercBuffer = new Byte[4 * 2048 * 2048];
  Byte* pDstArr     = new Byte[4 * 2048 * 2048];

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

    if (!lerc.GetLercInfo(pLercBuffer, fileSize, lercInfo))
      cout << "get header info failed" << endl;
    else
    {
      int w = lercInfo.nCols;
      int h = lercInfo.nRows;
      int nBands = lercInfo.nBands;
      Lerc::DataType dt = lercInfo.dt;

      pt.start();
      
      std::string resultMsg = "ok";
      BitMask bitMask;
      if (!lerc.Decode(pLercBuffer, fileSize, &bitMask, w, h, nBands, dt, (void*)pDstArr))
        resultMsg = "FAILED";

      pt.stop();
      printf("w = %4d, h = %4d, nBands = %2d, dt = %2d, time = %4d ms,  %s :  %s\n", w, h, nBands, (int)dt, pt.ms(), resultMsg.c_str(), fnVec[n].c_str());
    }
  }

#endif

  printf("\npress ENTER\n");
  getchar();
  
	return 0;
}

//-----------------------------------------------------------------------------
