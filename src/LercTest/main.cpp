
#include <tchar.h>
#include <iostream>
#include <math.h>
#include "BitMask.h"
#include "Lerc.h"

using namespace std;
using namespace LercNS;

//#define TestLegacyData

//------------------------------------------------------------------------------

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


  double ratio = w * h * (0.125 + sizeof(float)) / numBytesBlob;
  cout << "sample 1 compression ratio = " << ratio << endl;

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

  if (!lerc.Decode(pLercBlob, numBytesBlob, &bitMask3, w, h, 1, Lerc::DT_Float, (void*)zImg3))
    cout << "decode failed" << endl;


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

  cout << "max z error per pixel = " << maxDelta << endl;

  delete[] zImg;
  delete[] zImg3;
  delete[] pLercBlob;
  pLercBlob = 0;


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

  ratio = w * h * 3 / (double)numBytesBlob;
  cout << "sample 2 compression ratio = " << ratio << endl;

  // new data storage
  Byte* byteImg3 = new Byte[w * h * 3];
  memset(byteImg3, 0, w * h * 3);

  // decompress

  if (!lerc.GetLercInfo(pLercBlob, numBytesBlob, lercInfo))
    cout << "get header info failed" << endl;

  if (lercInfo.nCols != w || lercInfo.nRows != h || lercInfo.nBands != 3 || lercInfo.dt != Lerc::DT_Byte)
    cout << "got wrong lerc info" << endl;

  if (!lerc.Decode(pLercBlob, numBytesBlob, 0, w, h, 3, Lerc::DT_Byte, (void*)byteImg3))
    cout << "decode failed" << endl;

  // compare to orig

  maxDelta = 0;
  for (int k = 0, i = 0; i < h; i++)
    for (int j = 0; j < w; j++, k++)
    {
      double delta = abs(byteImg3[k] - byteImg[k]);
      if (delta > maxDelta)
        maxDelta = delta;
    }

  cout << "max z error per pixel = " << maxDelta << endl;

  delete[] byteImg;
  delete[] byteImg3;
  delete[] pLercBlob;
  pLercBlob = 0;

#ifdef TestLegacyData

  // Sample 3: decode old Lerc2 blob, float
  Byte* pBuffer = new Byte[2 * 1024 * 1024];
  string fn1 = "D:/tmaurer/Code/LercOpenSource/testData/testuv_w1922_h1083_float.lerc2";
  int nBytes = 1628068;
  {
    FILE* fp = 0;
    fopen_s(&fp, fn1.c_str(), "rb");
    fread(pBuffer, 1, nBytes, fp);
    fclose(fp);
  }

  if (!lerc.GetLercInfo(pBuffer, nBytes, lercInfo))
    cout << "get header info failed" << endl;

  w = lercInfo.nCols;
  h = lercInfo.nRows;
  int nBands = lercInfo.nBands;
  Lerc::DataType dt = lercInfo.dt;

  float* pArr = new float[w * h * nBands];

  if (!lerc.Decode(pBuffer, nBytes, 0, w, h, nBands, dt, (void*)pArr))
    cout << "decode failed" << endl;
  else
    cout << "decode succeeded" << endl;

  delete[] pArr;


  // Sample 4: decode old Lerc2 blob, ushort
  string fn2 = "D:/tmaurer/Code/LercOpenSource/testData/testuv_w1922_h1083_ushort.lerc2";
  nBytes = 1272601;
  {
    FILE* fp = 0;
    fopen_s(&fp, fn2.c_str(), "rb");
    fread(pBuffer, 1, nBytes, fp);
    fclose(fp);
  }

  if (!lerc.GetLercInfo(pBuffer, nBytes, lercInfo))
    cout << "get header info failed" << endl;

  w = lercInfo.nCols;
  h = lercInfo.nRows;
  nBands = lercInfo.nBands;
  dt = lercInfo.dt;

  unsigned short* pArr2 = new unsigned short[w * h * nBands];

  if (!lerc.Decode(pBuffer, nBytes, 0, w, h, nBands, dt, (void*)pArr2))
    cout << "decode failed" << endl;
  else
    cout << "decode succeeded" << endl;

  delete[] pArr2;


  // Sample 5: decode old Lerc1 blob
  string fn3 = "D:/tmaurer/Code/LercOpenSource/testData/amazon3.lerc1";
  nBytes = 121311;
  {
    FILE* fp = 0;
    fopen_s(&fp, fn3.c_str(), "rb");
    fread(pBuffer, 1, nBytes, fp);
    fclose(fp);
  }

  if (!lerc.GetLercInfo(pBuffer, nBytes, lercInfo))
    cout << "get header info failed" << endl;

  w = lercInfo.nCols;
  h = lercInfo.nRows;
  nBands = lercInfo.nBands;
  dt = lercInfo.dt;

  float* pArr3 = new float[w * h * nBands];

  if (!lerc.Decode(pBuffer, nBytes, 0, w, h, nBands, dt, (void*)pArr3))
    cout << "decode failed" << endl;
  else
    cout << "decode succeeded" << endl;

  delete[] pArr3;

#endif


  printf("\npress ENTER\n");
  getchar();
  
	return 0;
}

//------------------------------------------------------------------------------

