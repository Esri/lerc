
#include <cstring>
#include <cstdlib>
#include <cmath>
#include <vector>
#include <iostream>
#include <algorithm>
#include <chrono>
#include <cfloat>
#include "../LercLib/include/Lerc_c_api.h"
#include "../LercLib/include/Lerc_types.h"    // see for error codes, data types, etc

using namespace std;
using namespace std::chrono;

//#define TestLegacyData
//#define DumpDecodedTiles
//#define CompareAgainstDumpedTiles

typedef unsigned char Byte;    // convenience
typedef unsigned int uint32;
enum lerc_DataType { dt_char = 0, dt_uchar, dt_short, dt_ushort, dt_int, dt_uint, dt_float, dt_double };    // shorter

//-----------------------------------------------------------------------------
//    some helper functions
//-----------------------------------------------------------------------------

void BlobInfo_Print(const uint32* infoArr);
bool BlobInfo_Equal(const uint32* infoArr, uint32 nDepth, uint32 nCols, uint32 nRows, uint32 nBands, uint32 dataType);

#ifdef TestLegacyData
bool ReadListFile(const string& listFn, vector<string>& fnVec);
bool ReadFile(const string& fn, Byte* pBuffer, size_t bufferSize, size_t& nBytesRead);
bool WriteDecodedTile(const string& fn, int nMasks, const Byte* pValidBytes, int nDepth, int w, int h, int nBands, int bpp, const void* pArr);
bool ReadDecodedTile(const string& fn, int& nMasks, Byte* pValidBytes, int& nDepth, int& w, int& h, int& nBands, int& bpp, void* pArr);

bool CompareTwoTiles(int nMasks, const Byte* pValidBytes, int nDepth, int w, int h, int nBands, int bpp, const void* pArr,
  int nMasks2, const Byte* pValidBytes2, int nDepth2, int w2, int h2, int nBands2, int bpp2, const void* pArr2, int dt);
#endif

//-----------------------------------------------------------------------------
//    main
//-----------------------------------------------------------------------------

int main(int argc, char* arcv[])
{
  const int infoArrSize = (int)LercNS::InfoArrOrder::_last;
  const int dataRangeArrSize = (int)LercNS::DataRangeArrOrder::_last;

  lerc_status hr(0);
  uint32 infoArr[infoArrSize];
  double dataRangeArr[dataRangeArrSize];
  high_resolution_clock::time_point t0, t1;

#ifndef TestLegacyData

  //---------------------------------------------------------------------------

  // Sample 1: float image, 1 band, with some pixels set to invalid / void, maxZError = 0.1
  {
    int h = 512;
    int w = 512;

    float* zImg = new float[w * h];
    memset(zImg, 0, w * h * sizeof(float));

    Byte* maskByteImg = new Byte[w * h];
    memset(maskByteImg, 1, w * h);

    for (int k = 0, i = 0; i < h; i++)
      for (int j = 0; j < w; j++, k++)
      {
        zImg[k] = sqrt((float)(i * i + j * j));    // smooth surface
        zImg[k] += rand() % 20;    // add some small amplitude noise

        //if (i == 99 && j == 99)    // to test presence of NaN
        //  zImg[k] = NAN;

        if (j % 100 == 0 || i % 100 == 0)    // set some void pixels
          maskByteImg[k] = 0;
      }

    // compress into byte arr

    double maxZErrorWanted = 0.1;
    double eps = 0.0001;    // safety margin (optional), to account for finite floating point accuracy
    double maxZError = maxZErrorWanted - eps;

    uint32 numBytesNeeded = 0;
    uint32 numBytesWritten = 0;

    // optional, can call encode() directly with large enough blob buffer

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
    if ((hr = lerc_getBlobInfo(pLercBlob, numBytesBlob, infoArr, dataRangeArr, infoArrSize, dataRangeArrSize)))
      cout << "lerc_getBlobInfo(...) failed" << endl;

    BlobInfo_Print(infoArr);

    if (!BlobInfo_Equal(infoArr, 1, w, h, 1, (uint32)dt_float))
      cout << "got wrong lerc info" << endl;

    // new empty data storage
    float* zImg2 = new float[w * h];
    memset(zImg2, 0, w * h * sizeof(float));

    Byte* maskByteImg2 = new Byte[w * h];
    memset(maskByteImg2, 0, w * h);

    t0 = high_resolution_clock::now();

    hr = lerc_decode(pLercBlob, numBytesBlob,
      1,    // nMasks, added in Lerc 3.0 to allow for different masks per band (get it from infoArr via lerc_getBlobInfo(...))
      maskByteImg2, 1, w, h, 1, (uint32)dt_float, (void*)zImg2);

    if (hr)
      cout << "lerc_decode(...) failed" << endl;

    t1 = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(t1 - t0).count();

    // compare to orig

    double maxDelta = 0;
    for (int k = 0, i = 0; i < h; i++)
      for (int j = 0; j < w; j++, k++)
      {
        if (maskByteImg2[k] != maskByteImg[k])
          cout << "Error in main: decoded valid bytes differ from encoded valid bytes" << endl;

        if (maskByteImg2[k])
        {
          double delta = abs((double)zImg2[k] - (double)zImg[k]);
          if (delta > maxDelta)
            maxDelta = delta;
        }
      }

    cout << "max z error per pixel = " << maxDelta << ", decode time = " << duration << " ms" << endl;
    cout << endl;

    delete[] zImg;
    delete[] zImg2;
    delete[] maskByteImg;
    delete[] maskByteImg2;
    delete[] pLercBlob;
  }

  //---------------------------------------------------------------------------

  // Sample 2: random byte image, nDepth = 3, all pixels valid, maxZError = 0 (lossless)
  {
    int h = 713;
    int w = 257;

    Byte* byteImg = new Byte[3 * w * h];
    memset(byteImg, 0, 3 * w * h);

    for (int k = 0, i = 0; i < h; i++)
      for (int j = 0; j < w; j++, k++)
        for (int m = 0; m < 3; m++)
          byteImg[k * 3 + m] = rand() % 30;

    // encode
    uint32 numBytesWritten = 0;
    uint32 numBytesBlob = (3 * w * h) * 2;    // should be enough
    Byte* pLercBlob = new Byte[numBytesBlob];

    t0 = high_resolution_clock::now();

    hr = lerc_encode((void*)byteImg,    // raw image data: nDepth values per pixel, row by row, band by band
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
    auto duration = duration_cast<milliseconds>(t1 - t0).count();

    numBytesBlob = numBytesWritten;
    double ratio = 3 * w * h / (double)numBytesBlob;
    cout << "sample 2 compression ratio = " << ratio << ", encode time = " << duration << " ms" << endl;

    // decode

    if ((hr = lerc_getBlobInfo(pLercBlob, numBytesBlob, infoArr, dataRangeArr, infoArrSize, dataRangeArrSize)))
      cout << "lerc_getBlobInfo(...) failed" << endl;

    BlobInfo_Print(infoArr);

    if (!BlobInfo_Equal(infoArr, 3, w, h, 1, (uint32)dt_uchar))
      cout << "got wrong lerc info" << endl;

    vector<double> zMinVec(3, 0), zMaxVec(3, 0);
    if ((hr = lerc_getDataRanges(pLercBlob, numBytesBlob, 3, 1, &zMinVec[0], &zMaxVec[0])))
      cout << "lerc_getDataRanges(...) failed" << endl;

    // new data storage
    Byte* byteImg2 = new Byte[3 * w * h];
    memset(byteImg2, 0, 3 * w * h);

    t0 = high_resolution_clock::now();

    if ((hr = lerc_decode(pLercBlob, numBytesBlob, 0, nullptr, 3, w, h, 1, (uint32)dt_uchar, (void*)byteImg2)))
      cout << "lerc_decode(...) failed" << endl;

    t1 = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(t1 - t0).count();

    // compare to orig

    int maxDelta = 0;
    for (int k = 0, i = 0; i < h; i++)
      for (int j = 0; j < w; j++, k++)
        for (int m = 0; m < 3; m++)
        {
          int delta = abs((int)byteImg2[k * 3 + m] - (int)byteImg[k * 3 + m]);
          if (delta > maxDelta)
            maxDelta = delta;
        }

    cout << "max z error per pixel = " << maxDelta << ", decode time = " << duration << " ms" << endl;
    cout << endl;

    delete[] byteImg;
    delete[] byteImg2;
    delete[] pLercBlob;
  }

  //---------------------------------------------------------------------------

  // Sample 3: random float image, nBands = 4, no input mask, throw in some NaN pixels, maxZError = 0 (lossless)
  // Lerc will replace the NaN as invalid pixels using byte masks, one mask per band, as needed
  {
    int h = 128;
    int w = 257;

    float* fImg = new float[4 * w * h];
    memset(fImg, 0, 4 * w * h * sizeof(float));

    for (int iBand = 0; iBand < 4; iBand++)
    {
      float* arr = &fImg[iBand * w * h];

      for (int k = 0, i = 0; i < h; i++)
        for (int j = 0; j < w; j++, k++)
        {
          arr[k] = sqrt((float)(i * i + j * j));    // smooth surface
          arr[k] += rand() % 20;    // add some small amplitude noise

          if (iBand != 2 && rand() > 0.5 * RAND_MAX)   // set some NaN's but not for band 2
            arr[k] = NAN;
        }
    }

    // encode
    uint32 numBytesNeeded = 0;
    uint32 numBytesWritten = 0;

    // optional, can call encode directly with large enough buffer, 
    // but can be used to decide which method is best, Lerc or method bla

    hr = lerc_computeCompressedSize((void*)fImg,    // raw image data: nDepth values per pixel, row by row, band by band
      (uint32)dt_float, 1, w, h, 4,
      0,                   // nMasks, added in Lerc 3.0 to allow for different masks per band
      nullptr,             // can pass nullptr if all pixels are valid and nMasks = 0
      0,                   // max coding error per pixel
      &numBytesNeeded);    // size of outgoing Lerc blob

    if (hr)
      cout << "lerc_computeCompressedSize(...) failed" << endl;

    uint32 numBytesBlob = numBytesNeeded;
    Byte* pLercBlob = new Byte[numBytesBlob];

    t0 = high_resolution_clock::now();

    hr = lerc_encode((void*)fImg,    // raw image data: nDepth values per pixel, row by row, band by band
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
    auto duration = duration_cast<milliseconds>(t1 - t0).count();

    double ratio = 4 * w * h * sizeof(float) / (double)numBytesBlob;
    cout << "sample 3 compression ratio = " << ratio << ", encode time = " << duration << " ms" << endl;

    // decode

    if ((hr = lerc_getBlobInfo(pLercBlob, numBytesBlob, infoArr, dataRangeArr, infoArrSize, dataRangeArrSize)))
      cout << "lerc_getBlobInfo(...) failed" << endl;

    BlobInfo_Print(infoArr);

    if (!BlobInfo_Equal(infoArr, 1, w, h, 4, (uint32)dt_float))
      cout << "got wrong lerc info" << endl;

    // new data storage
    float* fImg2 = new float[4 * w * h];
    memset(fImg2, 0, 4 * w * h * sizeof(float));

    t0 = high_resolution_clock::now();

    int nMasks = infoArr[(int)LercNS::InfoArrOrder::nMasks];    // get the number of valid / invalid byte masks from infoArr

    Byte* maskByteImg2 = nullptr;

    if (nMasks > 0)    // possible values are 0, 1, or nBands
    {
      maskByteImg2 = new Byte[nMasks * w * h];
      memset(maskByteImg2, 0, nMasks * w * h);
    }

    if ((hr = lerc_decode(pLercBlob, numBytesBlob, nMasks, maskByteImg2, 1, w, h, 4, (uint32)dt_float, (void*)fImg2)))
      cout << "lerc_decode(...) failed" << endl;

    t1 = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(t1 - t0).count();

    // compare to orig

    double maxDelta = 0;
    for (int iBand = 0; iBand < 4; iBand++)
    {
      const float* arr = &fImg[iBand * w * h];
      const float* arr2 = &fImg2[iBand * w * h];
      const Byte* mask = (nMasks == 4) ? &maskByteImg2[iBand * w * h]
        : ((nMasks == 1) ? maskByteImg2 : nullptr);

      for (int k = 0, i = 0; i < h; i++)
        for (int j = 0; j < w; j++, k++)
          if (!mask || mask[k])
          {
            double delta = abs((double)arr2[k] - (double)arr[k]);
            if (delta > maxDelta || std::isnan(delta))
              maxDelta = delta;
          }
    }

    cout << "max z error per pixel = " << maxDelta << ", decode time = " << duration << " ms" << endl;
    cout << endl;

    delete[] fImg;
    delete[] fImg2;
    delete[] maskByteImg2;
    delete[] pLercBlob;
  }

  //---------------------------------------------------------------------------

  // Sample 4: float image, nBands = 2, nDepth = 2, pass noData value to band 0, maxZError = 0.001,
  // produce special mixed case of valid and invalid values at the same pixel;
  // so here we need to use a noData value as the 2D byte masks cannot cover this case. 

  // this example shows how to call the new Lerc API 4.0 functions which enable working with such mixed valid / invalid cases,
  // and allow to pass a noData value for that, one per band;
  {
    int h = 128;
    int w = 65;
    const int nBands = 2;
    const int nDepth = 2;  // might be a complex image
    int nValues = nDepth * w * h * nBands;

    float noDataVal = FLT_MAX;
    double maxZErr = 0.001;

    // here, the 1st band uses a noData value, the 2nd band does not; 
    // noData value can be passed in addition to a byte mask, it is not one or the other;
    Byte bUsesNoDataArr[nBands] = { 1, 0 };
    double noDataArr[nBands] = { noDataVal, 0 };

    float* fImg = new float[nValues];
    memset(fImg, 0, nValues * sizeof(float));

    for (int iBand = 0; iBand < nBands; iBand++)
    {
      float* arr = &fImg[nDepth * w * h * iBand];

      for (int k = 0, i = 0; i < h; i++)
        for (int j = 0; j < w; j++, k++)
        {
          int m = k * nDepth;
          arr[m] = sqrt((float)(i * i + j * j));    // smooth surface
          arr[m] += rand() % 20;    // add some small amplitude noise

          arr[m + 1] = arr[m];

          if (iBand == 0 && rand() > 0.5 * RAND_MAX)   // set some values to noData for band 0
            arr[m] = noDataVal;
        }
    }

    // encode
    uint32 numBytesNeeded = 0;
    uint32 numBytesWritten = 0;

    hr = lerc_computeCompressedSize_4D((void*)fImg,    // raw image data: nDepth values per pixel, row by row, band by band
      (uint32)dt_float, nDepth, w, h, nBands,
      0,                   // nMasks, added in Lerc 3.0 to allow for different masks per band
      nullptr,             // can pass nullptr if all pixels are valid and nMasks = 0
      maxZErr,             // max coding error per pixel
      &numBytesNeeded,     // size of outgoing Lerc blob
      bUsesNoDataArr,      // array of size nBands, set to 1 for each band that uses noData value
      noDataArr);          // array of size nBands, set noData value for each band that uses it

    if (hr)
      cout << "lerc_computeCompressedSize(...) failed" << endl;

    uint32 numBytesBlob = numBytesNeeded;
    Byte* pLercBlob = new Byte[numBytesBlob];

    t0 = high_resolution_clock::now();

    hr = lerc_encode_4D((void*)fImg,    // raw image data: nDepth values per pixel, row by row, band by band
      (uint32)dt_float, nDepth, w, h, nBands,
      0,                   // nMasks, added in Lerc 3.0 to allow for different masks per band
      nullptr,             // can pass nullptr if all pixels are valid and nMasks = 0
      maxZErr,             // max coding error per pixel
      pLercBlob,           // buffer to write to, function will fail if buffer too small
      numBytesBlob,        // buffer size
      &numBytesWritten,    // num bytes written to buffer
      bUsesNoDataArr,      // array of size nBands, set to 1 for each band that uses noData value
      noDataArr);          // array of size nBands, set noData value for each band that uses it

    if (hr)
      cout << "lerc_encode(...) failed" << endl;

    t1 = high_resolution_clock::now();
    auto duration = duration_cast<milliseconds>(t1 - t0).count();

    double ratio = nValues * sizeof(float) / (double)numBytesBlob;
    cout << "sample 4 compression ratio = " << ratio << ", encode time = " << duration << " ms" << endl;

    // decode

    if ((hr = lerc_getBlobInfo(pLercBlob, numBytesBlob, infoArr, dataRangeArr, infoArrSize, dataRangeArrSize)))
      cout << "lerc_getBlobInfo(...) failed" << endl;

    BlobInfo_Print(infoArr);

    if (!BlobInfo_Equal(infoArr, nDepth, w, h, nBands, (uint32)dt_float))
      cout << "got wrong lerc info" << endl;

    // new data storage
    float* fImg2 = new float[nValues];
    memset(fImg2, 0, nValues * sizeof(float));

    t0 = high_resolution_clock::now();

    // Lerc converts noData values to byte mask wherever possible
    int nMasks = infoArr[(int)LercNS::InfoArrOrder::nMasks];    // get the number of valid / invalid byte masks from infoArr
    int nUsesNoData = infoArr[(int)LercNS::InfoArrOrder::nUsesNoDataValue];    // 0 - noData value is not used, nBands - noData value used in one or more bands

    Byte* maskByteImg2 = nullptr;

    if (nMasks > 0)
    {
      maskByteImg2 = new Byte[nMasks * w * h];
      memset(maskByteImg2, 0, nMasks * w * h);
    }

    vector<Byte> bUsesNoDataVec(nBands, 0);
    vector<double> noDataVec(nBands, 0);

    hr = lerc_decode_4D(pLercBlob, numBytesBlob, nMasks, maskByteImg2, nDepth, w, h, nBands, (uint32)dt_float, (void*)fImg2,
      &bUsesNoDataVec[0], &noDataVec[0]);

    if (hr)
      cout << "lerc_decode(...) failed" << endl;

    t1 = high_resolution_clock::now();
    duration = duration_cast<milliseconds>(t1 - t0).count();

    // compare to orig

    double maxDelta = 0;
    bool bNoDataValueHasChanged = false;

    for (int iBand = 0; iBand < nBands; iBand++)
    {
      if (bUsesNoDataVec[iBand])  // this band uses a noData value; only possible if nDepth > 1 and some pixel has a mix of valid and invalid values
      {
        if (!bUsesNoDataArr[iBand])    // that would be a bug
          printf("Error: band %d changed from not using noData to using noData!\n", iBand);

        if (noDataVec[iBand] != noDataArr[iBand])
          printf("Error: noData value changed for band %d!\n", iBand);
      }

      const float* arr = &fImg[nDepth * w * h * iBand];
      const float* arr2 = &fImg2[nDepth * w * h * iBand];

      const Byte* mask = (nMasks == nBands) ? &maskByteImg2[iBand * w * h]
        : ((nMasks == 1) ? maskByteImg2 : nullptr);

      for (int k = 0, i = 0; i < h; i++)
        for (int j = 0; j < w; j++, k++)
          if (!mask || mask[k])
          {
            int m0 = k * nDepth;
            for (int m = m0; m < m0 + nDepth; m++)
            {
              double delta = abs((double)arr2[m] - (double)arr[m]);

              if (bUsesNoDataVec[iBand] && (arr[m] == noDataVal) && delta > 0)
                bNoDataValueHasChanged = true;

              if (delta > maxDelta)
                maxDelta = delta;
            }
          }
    }

    if (bNoDataValueHasChanged)
      cout << "Error: some noData value has changed!" << endl;

    cout << "max z error per pixel = " << maxDelta << ", decode time = " << duration << " ms" << endl;
    cout << endl;

    delete[] fImg;
    delete[] fImg2;
    delete[] maskByteImg2;
    delete[] pLercBlob;
  }

  //---------------------------------------------------------------------------
  //---------------------------------------------------------------------------

#endif // TestLegacyData

#ifdef TestLegacyData

  size_t lercBufferSize = 8 * 1024 * 1024;    // 8 MB
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

  string listFn = path + "_list.txt";
  if (!ReadListFile(listFn, fnVec))
    cout << "Read file " << listFn << " failed." << endl;

  //fnVec.clear();
  //fnVec.push_back("ShortBlob/broken_l2v2.lerc22");

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

    if ((hr = lerc_getBlobInfo(pLercBuffer, (uint32)fileSize, infoArr, dataRangeArr, infoArrSize, dataRangeArrSize)))
      cout << "lerc_getBlobInfo(...) failed" << endl;

    {
      int codecVersion = infoArr[0];
      uint32 dt = infoArr[1];
      int nDepth = infoArr[2];
      int w = infoArr[3];
      int h = infoArr[4];
      int nBands = infoArr[5];

      int nMasks = infoArr[(int)LercNS::InfoArrOrder::nMasks];    // get the number of valid / invalid byte masks from infoArr
      int nUsesNoData = infoArr[(int)LercNS::InfoArrOrder::nUsesNoDataValue];    // if > 0, noData value must be honored in addition to the byte mask

      double zMin = dataRangeArr[0];
      double zMax = dataRangeArr[1];

      const int bpp[] = { 1, 1, 2, 2, 4, 4, 4, 8 };

      Byte* pDstArr = new Byte[nBands * w * h * nDepth * bpp[dt]];
      Byte* pValidBytes = new Byte[nMasks * w * h];

      // allow for noData values, one per band
      Byte* pUsesNoData = new Byte[nBands];
      double* pNoDataVal = new double[nBands];

      t0 = high_resolution_clock::now();

      string resultMsg = "ok";
      if ((hr = lerc_decode_4D(pLercBuffer, (uint32)fileSize, nMasks, pValidBytes, nDepth, w, h, nBands, dt, (void*)pDstArr, pUsesNoData, pNoDataVal)))
        resultMsg = "FAILED";

      //memset(pDstArr, 5, nBands * w * h * nDepth * bpp[dt]);    // to double check on the CompareTwoTiles() function

      t1 = high_resolution_clock::now();
      auto duration = duration_cast<milliseconds>(t1 - t0).count();
      totalDecodeTime += duration;

      printf("codec = %1d, nDepth = %1d, w = %4d, h = %4d, nBands = %1d, nMasks = %1d, usesNoData = %1d, dt = %1d, min = %10.4lf, max = %16.4lf, time = %4d ms,  %s :  %s\n",
          codecVersion, nDepth, w, h, nBands, nMasks, nUsesNoData ? 1 : 0, dt, zMin, zMax, (int)duration, resultMsg.c_str(), fnVec[n].c_str());

      string fnDec = pathDecoded;
      string s = fnVec[n];
      replace(s.begin(), s.end(), '/', '_');
      fnDec += s;

#ifdef DumpDecodedTiles
      if (!WriteDecodedTile(fnDec, nMasks, pValidBytes, nDepth, w, h, nBands, bpp[dt], pDstArr))
        printf("Write decoded Lerc tile failed for %s\n", fnDec.c_str());
#endif

#ifdef CompareAgainstDumpedTiles
      Byte* pDstArr2 = new Byte[nBands * w * h * nDepth * bpp[dt]];
      Byte* pValidBytes2 = new Byte[nMasks * w * h];

      int nMasks2(0), nDepth2(0), w2(0), h2(0), nBands2(0), bpp2(0);
      if (!ReadDecodedTile(fnDec, nMasks2, pValidBytes2, nDepth2, w2, h2, nBands2, bpp2, pDstArr2))
        printf("Read decoded Lerc tile failed for %s\n", fnDec.c_str());

      if (!CompareTwoTiles(nMasks, pValidBytes, nDepth, w, h, nBands, bpp[dt], pDstArr,
        nMasks2, pValidBytes2, nDepth2, w2, h2, nBands2, bpp2, pDstArr2, dt))
        printf("Compare two Lerc tiles failed for %s vs %s\n", fnVec[n].c_str(), fnDec.c_str());

      delete[] pDstArr2;
      delete[] pValidBytes2;
#endif

      delete[] pDstArr;
      delete[] pValidBytes;

      delete[] pUsesNoData;
      delete[] pNoDataVal;
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
//-----------------------------------------------------------------------------

void BlobInfo_Print(const uint32* infoArr)
{
  const uint32* ia = infoArr;
  printf("version = %d, dataType = %d, nDepth = %d, nCols = %d, nRows = %d, nBands = %d, \
    nValidPixels = %d, blobSize = %d, nMasks = %d, nDepth = %d, nUsesNoDataValue = %d\n",
    ia[0], ia[1], ia[2], ia[3], ia[4], ia[5], ia[6], ia[7], ia[8], ia[9], ia[10]);
}

bool BlobInfo_Equal(const uint32* infoArr, uint32 nDepth, uint32 nCols, uint32 nRows, uint32 nBands, uint32 dataType)
{
  const uint32* ia = infoArr;
  return ia[1] == dataType && ia[2] == nDepth && ia[3] == nCols && ia[4] == nRows && ia[5] == nBands;
}

//-----------------------------------------------------------------------------

#ifdef TestLegacyData

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
  int rv = fscanf(fp, "%d", &num);
  for (int i = 0; i < num; i++)
  {
    rv = fscanf(fp, "%s", buffer);
    string s = buffer;
    fnVec.push_back(s);
  }

  fclose(fp);
  return true;
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
    printf("Lerc blob size %d > buffer size of %d\n", (uint32)fileSize, (uint32)bufferSize);
    return false;
  }

  fp = fopen(fn.c_str(), "rb");
  nBytesRead = fread(pBuffer, 1, fileSize, fp);    // read Lerc blob into buffer
  fclose(fp);
  return nBytesRead == fileSize;
}

bool WriteDecodedTile(const string& fn, int nMasks, const Byte* pValidBytes, int nDepth, int w, int h, int nBands, int bpp, const void* pArr)
{
  FILE* fp = fopen(fn.c_str(), "wb");
  if (!fp)
  {
    printf("Cannot open file %s\n", fn.c_str());
    return false;
  }

  int dimensions[] = { nMasks, nDepth, w, h, nBands, bpp };
  fwrite(dimensions, 1, 6 * sizeof(int), fp);

  if (nMasks > 0)
    fwrite(pValidBytes, 1, nMasks * w * h, fp);

  fwrite(pArr, 1, nBands * w * h * nDepth * bpp, fp);
  fclose(fp);
  return true;
}

bool ReadDecodedTile(const string& fn, int& nMasks, Byte* pValidBytes, int& nDepth, int& w, int& h, int& nBands, int& bpp, void* pArr)
{
  FILE* fp = fopen(fn.c_str(), "rb");
  if (!fp)
  {
    printf("Cannot open file %s\n", fn.c_str());
    return false;
  }

  int dims[6] = { 0 };
  size_t nbRead = fread(dims, 1, 6 * sizeof(int), fp);
  nMasks = dims[0];
  nDepth = dims[1];
  w = dims[2];
  h = dims[3];
  nBands = dims[4];
  bpp = dims[5];

  if (nMasks > 0)
    nbRead += fread(pValidBytes, 1, nMasks * w * h, fp);

  nbRead += fread(pArr, 1, nBands * w * h * nDepth * bpp, fp);
  fclose(fp);
  return true;
}

bool CompareTwoTiles(int nMasks, const Byte* pValidBytes, int nDepth, int w, int h, int nBands, int bpp, const void* pArr,
  int nMasks2, const Byte* pValidBytes2, int nDepth2, int w2, int h2, int nBands2, int bpp2, const void* pArr2, int dt)
{
  if (nMasks2 != nMasks || nDepth2 != nDepth || w2 != w || h2 != h || nBands2 != nBands || bpp2 != bpp)
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
          for (int m = 0; m < nDepth; m++)
          {
            int index = (iBand * w * h + k) * nDepth + m;
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

#endif

//-----------------------------------------------------------------------------

