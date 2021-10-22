
#include <cstring>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>
#include "../LercLib/include/Lerc_c_api.h"
//#include "../LercLib/include/Lerc_types.h"    // see for error codes, data types, etc

using namespace std;
using namespace std::chrono;

//#define TestLegacyData
//#define DumpDecodedTiles
//#define CompareAgainstDumpedTiles

typedef unsigned char Byte;    // convenience
typedef unsigned int uint32;

//-----------------------------------------------------------------------------

enum lerc_DataType { dt_char = 0, dt_uchar, dt_short, dt_ushort, dt_int, dt_uint, dt_float, dt_double };

void BlobInfo_Print(const uint32* infoArr)
{
  const uint32* ia = infoArr;
  printf("version = %d, dataType = %d, nDim = %d, nCols = %d, nRows = %d, nBands = %d, nValidPixels = %d, blobSize = %d, nMasks = %d\n",
    ia[0], ia[1], ia[2], ia[3], ia[4], ia[5], ia[6], ia[7], ia[8]);
}

bool BlobInfo_Equal(const uint32* infoArr, uint32 nDim, uint32 nCols, uint32 nRows, uint32 nBands, uint32 dataType)
{
  const uint32* ia = infoArr;
  return ia[1] == dataType && ia[2] == nDim && ia[3] == nCols && ia[4] == nRows && ia[5] == nBands;
}

bool ReadFile(const string& fn, Byte* pBuffer, size_t bufferSize, size_t& nBytesRead)
{
  nBytesRead = 0;
  FILE* fp = fopen(fn.c_str(), "rb");
  if (!fp)
  {
    printf("Cannot open file %s\n", fn.c_str());
    return false;
  }

  fseek(fp, 0, SEEK_END);
  size_t fileSize = ftell(fp);    // get the file size
  fclose(fp);
  fp = 0;

  if (fileSize > bufferSize)
  {
    printf("Lerc blob size %lu > buffer size of %lu\n", (uint32)fileSize, (uint32)bufferSize);
    return false;
  }

  fp = fopen(fn.c_str(), "rb");
  nBytesRead = fread(pBuffer, 1, fileSize, fp);    // read Lerc blob into buffer
  fclose(fp);
  return nBytesRead == fileSize;
}

bool WriteDecodedTile(const string& fn, int nMasks, const Byte* pValidBytes, int nDim, int w, int h, int nBands, int bpp, const void* pArr)
{
  FILE* fp = fopen(fn.c_str(), "wb");
  if (!fp)
  {
    printf("Cannot open file %s\n", fn.c_str());
    return false;
  }

  int dimensions[] = { nMasks, nDim, w, h, nBands, bpp };
  fwrite(dimensions, 1, 6 * sizeof(int), fp);

  if (nMasks > 0)
    fwrite(pValidBytes, 1, nMasks * w * h, fp);

  fwrite(pArr, 1, nBands * w * h * nDim * bpp, fp);
  fclose(fp);
  return true;
}

bool ReadDecodedTile(const string& fn, int& nMasks, Byte* pValidBytes, int& nDim, int& w, int& h, int& nBands, int& bpp, void* pArr)
{
  FILE* fp = fopen(fn.c_str(), "rb");
  if (!fp)
  {
    printf("Cannot open file %s\n", fn.c_str());
    return false;
  }

  int dims[6] = { 0 };
  fread(dims, 1, 6 * sizeof(int), fp);
  nMasks = dims[0];
  nDim = dims[1];
  w = dims[2];
  h = dims[3];
  nBands = dims[4];
  bpp = dims[5];

  if (nMasks > 0)
    fread(pValidBytes, 1, nMasks * w * h, fp);

  fread(pArr, 1, nBands * w * h * nDim * bpp, fp);
  fclose(fp);
  return true;
}

bool CompareTwoTiles(int nMasks, const Byte* pValidBytes, int nDim, int w, int h, int nBands, int bpp, const void* pArr,
  int nMasks2, const Byte* pValidBytes2, int nDim2, int w2, int h2, int nBands2, int bpp2, const void* pArr2, int dt)
{
  if (nMasks2 != nMasks || nDim2 != nDim || w2 != w || h2 != h || nBands2 != nBands || bpp2 != bpp)
  {
    printf("Tile dimensions differ.\n");
    return false;
  }
  if (0 != memcmp(pValidBytes2, pValidBytes, nMasks * w * h))
  {
    printf("Valid byte masks differ.\n");
    return false;
  }

  double maxDelta(0);

  for (int iBand = 0; iBand < nBands; iBand++)
  {
    const Byte* mask = (nMasks == nBands) ? &pValidBytes[iBand * w * h] : (nMasks == 1 ? pValidBytes : nullptr);

    for (int k = 0, i = 0; i < h; i++)
    {
      for (int j = 0; j < w; j++, k++)
      {
        if (!mask || mask[k])
        {
          for (int m = 0; m < nDim; m++)
          {
            int index = (iBand * w * h + k) * nDim + m;
            double delta = 0;

            const Byte* ptr = (const Byte*)pArr + index * bpp;
            const Byte* ptr2 = (const Byte*)pArr2 + index * bpp;

            if (dt <= 5)
            {
              if (0 != memcmp(ptr, ptr2, bpp))
              {
                printf("Data differ.\n");
                return false;
              }
            }
            else if (dt == 6)
            {
              float x(0), x2(0);
              memcpy(&x, ptr, bpp);
              memcpy(&x2, ptr2, bpp);
              delta = fabs(x2 - x);
            }
            else if (dt == 7)
            {
              double x(0), x2(0);
              memcpy(&x, ptr, bpp);
              memcpy(&x2, ptr2, bpp);
              delta = fabs(x2 - x);
            }

            if (delta > maxDelta)
              maxDelta = delta;
          }
        }
      }
    }
  }

  if (maxDelta > 0)
  {
    printf("Valid pixel values differ, max delta = %lf.\n", maxDelta);
    return false;
  }

  return true;
}

bool ReadListFile(const string& listFn, vector<string>& fnVec)
{
  FILE* fp = fopen(listFn.c_str(), "r");
  if (!fp)
  {
    printf("Cannot open file %s\n", listFn.c_str());
    return false;
  }

  char buffer[1024];
  int num = 0;
  fscanf(fp, "%d", &num);
  for (int i = 0; i < num; i++)
  {
    fscanf(fp, "%s", buffer);
    string s = buffer;
    fnVec.push_back(s);
  }

  fclose(fp);
  return true;
}

//-----------------------------------------------------------------------------

int main(int argc, char* arcv[])
{
  lerc_status hr(0);
  uint32 infoArr[10];
  double dataRangeArr[3];
  high_resolution_clock::time_point t0, t1;

#ifndef TestLegacyData

  // Sample 1: float image, 1 band, with some pixels set to invalid / void, maxZError = 0.1

  int h = 512;
  int w = 512;

  float* zImg = new float[w * h];
  memset(zImg, 0, w * h * sizeof(float));

  Byte* maskByteImg = new Byte[w * h];
  memset(maskByteImg, 1, w * h);

  for (int k = 0, i = 0; i < h; i++)
  {
    for (int j = 0; j < w; j++, k++)
    {
      zImg[k] = sqrt((float)(i * i + j * j));    // smooth surface
      zImg[k] += rand() % 20;    // add some small amplitude noise

      //if (i == 99 && j == 99)
      //  zImg[k] = NAN;

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
    1,                   // nMasks, added in Lerc 3.0 to allow for different masks per band
    maskByteImg,         // can pass nullptr if all pixels are valid and nMasks = 0
    maxZError,           // max coding error per pixel, or precision
    &numBytesNeeded);    // size of outgoing Lerc blob

  if (hr)
    cout << "lerc_computeCompressedSize(...) failed" << endl;

  uint32 numBytesBlob = numBytesNeeded;
  Byte* pLercBlob = new Byte[numBytesBlob];

  t0 = high_resolution_clock::now();

  hr = lerc_encode((void*)zImg,    // raw image data, row by row, band by band
    (uint32)dt_float, 1, w, h, 1,
    1,                   // nMasks, added in Lerc 3.0 to allow for different masks per band
    maskByteImg,         // can pass nullptr if all pixels are valid and nMasks = 0
    maxZError,           // max coding error per pixel, or precision
    pLercBlob,           // buffer to write to, function will fail if buffer too small
    numBytesBlob,        // buffer size
    &numBytesWritten);   // num bytes written to buffer

  if (hr)
    cout << "lerc_encode(...) failed" << endl;

  t1 = high_resolution_clock::now();
  auto duration = duration_cast<milliseconds>(t1 - t0).count();

  double ratio = w * h * (0.125 + sizeof(float)) / numBytesBlob;
  cout << "sample 1 compression ratio = " << ratio << ", encode time = " << duration << " ms" << endl;


  // decompress
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

  hr = lerc_decode(pLercBlob, numBytesBlob,
    1,    // nMasks, added in Lerc 3.0 to allow for different masks per band (get it from infoArr via lerc_getBlobInfo(...))
    maskByteImg3, 1, w, h, 1, (uint32)dt_float, (void*)zImg3);

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
    0,                   // nMasks, added in Lerc 3.0 to allow for different masks per band
    nullptr,             // can pass nullptr if all pixels are valid and nMasks = 0
    0,                   // max coding error per pixel
    &numBytesNeeded);    // size of outgoing Lerc blob

  if (hr)
    cout << "lerc_computeCompressedSize(...) failed" << endl;

  numBytesBlob = numBytesNeeded;
  pLercBlob = new Byte[numBytesBlob];

  t0 = high_resolution_clock::now();

  hr = lerc_encode((void*)byteImg,    // raw image data: nDim values per pixel, row by row, band by band
    (uint32)dt_uchar, 3, w, h, 1,
    0,                   // nMasks, added in Lerc 3.0 to allow for different masks per band
    nullptr,             // can pass nullptr if all pixels are valid and nMasks = 0
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

  vector<double> zMinVec(3, 0), zMaxVec(3, 0);
  hr = lerc_getDataRanges(pLercBlob, numBytesBlob, 3, 1, &zMinVec[0], &zMaxVec[0]);
  if (hr)
    cout << "lerc_getDataRanges(...) failed" << endl;

  // new data storage
  Byte* byteImg3 = new Byte[3 * w * h];
  memset(byteImg3, 0, 3 * w * h);

  t0 = high_resolution_clock::now();

  hr = lerc_decode(pLercBlob, numBytesBlob, 0, nullptr, 3, w, h, 1, (uint32)dt_uchar, (void*)byteImg3);
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

  // Sample 3: random float image, nBands = 4, no input mask, throw in some NaN pixels, maxZError = 0 (lossless)
  // Lerc will replace the NaN as invalid pixels using byte masks, one mask per band, as needed

  h = 128;
  w = 257;

  float* fImg = new float[4 * w * h];
  memset(fImg, 0, 4 * w * h * sizeof(float));

  for (int iBand = 0; iBand < 4; iBand++)
  {
    float* arr = &fImg[iBand * w * h];

    for (int k = 0, i = 0; i < h; i++)
    {
      for (int j = 0; j < w; j++, k++)
      {
        arr[k] = sqrt((float)(i * i + j * j));    // smooth surface
        arr[k] += rand() % 20;    // add some small amplitude noise

        if (iBand != 2 && rand() > 0.5 * RAND_MAX)   // set some NaN's but not for band 2
          arr[k] = NAN;
      }
    }
  }


  // encode

  hr = lerc_computeCompressedSize((void*)fImg,    // raw image data: nDim values per pixel, row by row, band by band
    (uint32)dt_float, 1, w, h, 4,
    0,                   // nMasks, added in Lerc 3.0 to allow for different masks per band
    nullptr,             // can pass nullptr if all pixels are valid and nMasks = 0
    0,                   // max coding error per pixel
    &numBytesNeeded);    // size of outgoing Lerc blob

  if (hr)
    cout << "lerc_computeCompressedSize(...) failed" << endl;

  numBytesBlob = numBytesNeeded;
  pLercBlob = new Byte[numBytesBlob];

  t0 = high_resolution_clock::now();

  hr = lerc_encode((void*)fImg,    // raw image data: nDim values per pixel, row by row, band by band
    (uint32)dt_float, 1, w, h, 4,
    0,                   // nMasks, added in Lerc 3.0 to allow for different masks per band
    nullptr,             // can pass nullptr if all pixels are valid and nMasks = 0
    0,                   // max coding error per pixel
    pLercBlob,           // buffer to write to, function will fail if buffer too small
    numBytesBlob,        // buffer size
    &numBytesWritten);   // num bytes written to buffer

  if (hr)
    cout << "lerc_encode(...) failed" << endl;

  t1 = high_resolution_clock::now();
  duration = duration_cast<milliseconds>(t1 - t0).count();

  ratio = 4 * w * h * sizeof(float) / (double)numBytesBlob;
  cout << "sample 3 compression ratio = " << ratio << ", encode time = " << duration << " ms" << endl;


  // decode

  hr = lerc_getBlobInfo(pLercBlob, numBytesBlob, infoArr, dataRangeArr, 10, 3);
  if (hr)
    cout << "lerc_getBlobInfo(...) failed" << endl;

  BlobInfo_Print(infoArr);

  if (!BlobInfo_Equal(infoArr, 1, w, h, 4, (uint32)dt_float))
    cout << "got wrong lerc info" << endl;

  // new data storage
  float* fImg3 = new float[4 * w * h];
  memset(fImg3, 0, 4 * w * h * sizeof(float));

  t0 = high_resolution_clock::now();

  int nMasks = infoArr[8];    // get the number of valid / invalid byte masks from infoArr

  if (nMasks > 0)
  {
    maskByteImg3 = new Byte[nMasks * w * h];
    memset(maskByteImg3, 0, nMasks * w * h);
  }
  else
    maskByteImg3 = nullptr;

  hr = lerc_decode(pLercBlob, numBytesBlob, nMasks, maskByteImg3, 1, w, h, 4, (uint32)dt_float, (void*)fImg3);
  if (hr)
    cout << "lerc_decode(...) failed" << endl;

  t1 = high_resolution_clock::now();
  duration = duration_cast<milliseconds>(t1 - t0).count();

  // compare to orig

  maxDelta = 0;
  for (int iBand = 0; iBand < 4; iBand++)
  {
    const float* arr = &fImg[iBand * w * h];
    const float* arr3 = &fImg3[iBand * w * h];
    const Byte* mask = (nMasks == 4) ? &maskByteImg3[iBand * w * h]
      : ((nMasks == 1) ? &maskByteImg3[w * h] : nullptr);

    for (int k = 0, i = 0; i < h; i++)
      for (int j = 0; j < w; j++, k++)
        if (!mask || mask[k])
        {
          double delta = abs(arr3[k] - arr[k]);
          if (delta > maxDelta || std::isnan(delta))
            maxDelta = delta;
        }
  }

  cout << "max z error per pixel = " << maxDelta << ", decode time = " << duration << " ms" << endl;
  cout << endl;

  delete[] fImg;
  delete[] fImg3;
  delete[] maskByteImg3;
  delete[] pLercBlob;
  pLercBlob = 0;

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------

#endif // TestLegacyData

#ifdef TestLegacyData

  size_t lercBufferSize = 8 * 1024 * 1024;
  Byte* pLercBuffer = new Byte[lercBufferSize];

  double totalDecodeTime = 0;

   vector<string> fnVec;
  string path = "D:/GitHub/LercOpenSource_v2.5/testData/";    // windows
  string pathDecoded = "C:/Temp/LercDecodedTiles/";
  //string path = "/home/john/Documents/Code/TestData/";    // ubuntu
  //string pathDecoded = "/home/john/Documents/Code/DecodedTiles/";
#ifdef USE_EMSCRIPTEN
  path = "PreLoadedFiles/LercBlobs/";
  pathDecoded = "PreLoadedFiles/DecodedTiles/";
#endif

  string listFn = path + "list.txt";
  if (!ReadListFile(listFn, fnVec))
    cout << "Read file " << listFn << " failed." << endl;

  //fnVec.clear();
  //fnVec.push_back("bluemarble_256_256.lerc2");

  for (size_t n = 0; n < fnVec.size(); n++)
  {
    string fn = path;
    fn += fnVec[n];

    size_t fileSize = 0;
    if (!ReadFile(fn, pLercBuffer, lercBufferSize, fileSize))
    {
      cout << "Read file " << fn << " failed." << endl;
      continue;
    }

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
      int nMasks = infoArr[8];
      double zMin = dataRangeArr[0];
      double zMax = dataRangeArr[1];

      vector<double> zMinVec(nDim * nBands, 0), zMaxVec(nDim * nBands, 0);
      hr = lerc_getDataRanges(pLercBuffer, (uint32)fileSize, nDim, nBands, &zMinVec[0], &zMaxVec[0]);
      if (hr)
        cout << "lerc_getDataRanges(...) failed" << endl;

      double minFound = *std::min_element(zMinVec.begin(), zMinVec.end());
      double maxFound = *std::max_element(zMaxVec.begin(), zMaxVec.end());
      if (minFound != zMin)
        cout << "data ranges overall min differs from overall min: " << minFound << " vs " << zMin << endl;
      if (maxFound != zMax)
        cout << "data ranges overall max differs from overall max: " << maxFound << " vs " << zMax << endl;

      const int bpp[] = { 1, 1, 2, 2, 4, 4, 4, 8 };

      Byte* pDstArr = new Byte[nBands * w * h * nDim * bpp[dt]];
      Byte* pValidBytes = new Byte[nMasks * w * h];

      t0 = high_resolution_clock::now();

      std::string resultMsg = "ok";
      if (0 != lerc_decode(pLercBuffer, (uint32)fileSize, nMasks, pValidBytes, nDim, w, h, nBands, dt, (void*)pDstArr))
        resultMsg = "FAILED";

      //memset(pDstArr, 5, nBands * w * h * nDim * bpp[dt]);    // to double check on the CompareTwoTiles() function

      t1 = high_resolution_clock::now();
      auto duration = duration_cast<milliseconds>(t1 - t0).count();
      totalDecodeTime += duration;

      printf("nDim = %1d, w = %4d, h = %4d, nBands = %1d, nMasks = %1d, dt = %1d, min = %10.4f, max = %16.4f, time = %3lld ms,  %s :  %s\n",
          nDim, w, h, nBands, nMasks, dt, zMin, zMax, duration, resultMsg.c_str(), fnVec[n].c_str());

      string fnDec = pathDecoded;
      string s = fnVec[n];
      replace(s.begin(), s.end(), '/', '_');
      fnDec += s;

#ifdef DumpDecodedTiles
      if (!WriteDecodedTile(fnDec, nMasks, pValidBytes, nDim, w, h, nBands, bpp[dt], pDstArr))
        printf("Write decoded Lerc tile failed for %s\n", fnDec.c_str());
#endif

#ifdef CompareAgainstDumpedTiles
      Byte* pDstArr2 = new Byte[nBands * w * h * nDim * bpp[dt]];
      Byte* pValidBytes2 = new Byte[nMasks * w * h];

      int nMasks2(0), nDim2(0), w2(0), h2(0), nBands2(0), bpp2(0);
      if (!ReadDecodedTile(fnDec, nMasks2, pValidBytes2, nDim2, w2, h2, nBands2, bpp2, pDstArr2))
        printf("Read decoded Lerc tile failed for %s\n", fnDec.c_str());

      if (!CompareTwoTiles(nMasks, pValidBytes, nDim, w, h, nBands, bpp[dt], pDstArr,
        nMasks2, pValidBytes2, nDim2, w2, h2, nBands2, bpp2, pDstArr2, dt))
        printf("Compare two Lerc tiles failed for %s vs %s\n", fnVec[n].c_str(), fnDec.c_str());

      delete[] pDstArr2;
      delete[] pValidBytes2;
#endif

      delete[] pDstArr;
      delete[] pValidBytes;
    }
  }

  delete[] pLercBuffer;

  printf("Total decode time = %d ms", (int)totalDecodeTime);

#endif

  printf("\npress ENTER\n");
  getchar();

  return 0;
}

//-----------------------------------------------------------------------------
