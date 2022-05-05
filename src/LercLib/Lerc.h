/*
Copyright 2015 - 2022 Esri

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

A local copy of the license and additional notices are located with the
source distribution at:

http://github.com/Esri/lerc/

Contributors:  Thomas Maurer
*/

#pragma once

#include <cstring>
#include <vector>
#include "include/Lerc_types.h"
#include "BitMask.h"
#include "Lerc2.h"

NAMESPACE_LERC_START

#ifdef HAVE_LERC1_DECODE
  class CntZImage;
#endif

  class Lerc
  {
  public:
    Lerc() {}
    ~Lerc() {}

    // data types supported by Lerc
    enum DataType { DT_Char, DT_Byte, DT_Short, DT_UShort, DT_Int, DT_UInt, DT_Float, DT_Double, DT_Undefined };

    // all functions are provided in 2 flavors
    // - using void pointers to the image data, can be called on a Lerc lib or dll
    // - data templated, can be called if compiled together


    // Encode

    // if more than 1 band, the outgoing Lerc blob has the single band Lerc blobs concatenated; 
    // or, if you have multiple values per pixel and stored as [RGB, RGB, ... ], then set nDepth accordingly (e.g., 3)

    // computes the number of bytes needed to allocate the buffer, accurate to the byte;
    // does not encode the image data, but uses statistics and formulas to compute the buffer size needed;
    // this function is optional, you can also use a buffer large enough to call Encode() directly, 
    // or, if encoding a batch of same width / height tiles, call this function once, double the buffer size, and
    // then just call Encode() on all tiles;

    static ErrCode ComputeCompressedSize(
      const void* pData,               // raw image data, row by row, band by band
      int version,                     // 2 = v2.2, 3 = v2.3, 4 = v2.4, 5 = v2.5
      DataType dt,                     // data type, char to double
      int nDepth,                      // number of values per pixel
      int nCols,                       // number of cols
      int nRows,                       // number of rows
      int nBands,                      // number of bands
      int nMasks,                      // number of masks (0, 1, or nBands)
      const Byte* pValidBytes,         // masks (size = nMasks * nRows * nCols)
      double maxZErr,                  // max coding error per pixel, defines the precision
      unsigned int& numBytesNeeded,    // size of outgoing Lerc blob
      const unsigned char* pUsesNoData,// if there are invalid values not marked by the mask, pass an array of size nBands, 1 - uses noData, 0 - not
      const double* noDataValues);     // same, pass an array of size nBands with noData value per band, or pass nullptr

    // encodes or compresses the image data into the buffer

    static ErrCode Encode(
      const void* pData,               // raw image data, row by row, band by band
      int version,                     // 2 = v2.2, 3 = v2.3, 4 = v2.4, 5 = v2.5
      DataType dt,                     // data type, char to double
      int nDepth,                      // number of values per pixel
      int nCols,                       // number of cols
      int nRows,                       // number of rows
      int nBands,                      // number of bands
      int nMasks,                      // number of masks (0, 1, or nBands)
      const Byte* pValidBytes,         // masks (size = nMasks * nRows * nCols)
      double maxZErr,                  // max coding error per pixel, defines the precision
      Byte* pBuffer,                   // buffer to write to, function fails if buffer too small
      unsigned int numBytesBuffer,     // buffer size
      unsigned int& numBytesWritten,   // num bytes written to buffer
      const unsigned char* pUsesNoData,// if there are invalid values not marked by the mask, pass an array of size nBands, 1 - uses noData, 0 - not
      const double* noDataValues);     // same, pass an array of size nBands with noData value per band, or pass nullptr

    // Decode

    struct LercInfo
    {
      int version,        // Lerc version number (0 for old Lerc1, 1 to 5 for Lerc 2.1 to 2.5)
        nDepth,           // number of values per pixel
        nCols,            // number of columns
        nRows,            // number of rows
        numValidPixel,    // number of valid pixels
        nBands,           // number of bands
        blobSize,         // total blob size in bytes
        nMasks,           // number of masks (0, 1, or nBands)
        nUsesNoDataValue; // 0 - no noData value used, nBands - noData value used in 1 or more bands (only possible for nDepth > 1)
      DataType dt;        // data type (float only for old Lerc1)
      double zMin,        // min pixel value, over all data values
        zMax,             // max pixel value, over all data values
        maxZError;        // maxZError used for encoding

      void RawInit()  { memset(this, 0, sizeof(struct LercInfo)); }
    };

    // again, this function is optional;
    // call it on a Lerc blob to get the above struct returned, from this the data arrays
    // can be constructed before calling Decode();
    // same as above, for a batch of Lerc blobs of the same kind, you can call this function on 
    // the first blob, get the info, and on the other Lerc blobs just call Decode();
    // this function is very fast on (newer) Lerc2 blobs as it only reads the blob headers;

    // Remark on param numBytesBlob. Usually it is known, either the file size of the blob written to disk, 
    // or the size of the blob transmitted. It should be accurate for 2 reasons:
    // _ function finds out how many single band Lerc blobs are concatenated
    // _ function checks for truncated file or blob
    // It is OK to pass numBytesBlob too large as long as there is no other (valid) Lerc blob following next.
    // If in doubt, check the code in Lerc::GetLercInfo(...) for the exact logic. 

    static ErrCode GetLercInfo(const Byte* pLercBlob,       // Lerc blob to decode
      unsigned int numBytesBlob,   // size of Lerc blob in bytes
      struct LercInfo& lercInfo,
      double* pMins = nullptr,     // pass array of size (nDepth * nBands) to get the min values per dimension and band
      double* pMaxs = nullptr,     // same as pMins, to get the max values
      size_t nElem = 0);           // (nDepth * nBands), if passed

    // setup outgoing arrays accordingly, then call Decode()

    static ErrCode Decode(
      const Byte* pLercBlob,           // Lerc blob to decode
      unsigned int numBytesBlob,       // size of Lerc blob in bytes
      int nMasks,                      // number of masks (0, 1, or nBands)
      Byte* pValidBytes,               // masks (fails if not big enough to take the masks decoded, fills with 1 if all valid)
      int nDepth,                      // number of values per pixel
      int nCols,                       // number of cols
      int nRows,                       // number of rows
      int nBands,                      // number of bands
      DataType dt,                     // data type of outgoing array
      void* pData,                     // outgoing data bands
      unsigned char* pUsesNoData,      // pass an array of size nBands, 1 - band uses noData, 0 - not
      double* noDataValues);           // same, pass an array of size nBands to get the noData value per band, if any

    static ErrCode ConvertToDouble(
      const void* pDataIn,             // pixel data of image tile of data type dt (< double)
      DataType dt,                     // data type of input data
      size_t nDataValues,              // total number of data values (nDepth * nCols * nRows * nBands)
      double* pDataOut);               // pixel data converted to double


    // same as functions above, but data templated instead of using void pointers

    template<class T> static ErrCode ComputeCompressedSizeTempl(
      const T* pData,                  // raw image data, row by row, band by 
      int version,                     // 2 = v2.2, 3 = v2.3, 4 = v2.4, 5 = v2.5
      int nDepth,                      // number of values per pixel
      int nCols,                       // number of cols
      int nRows,                       // number of rows
      int nBands,                      // number of bands
      int nMasks,                      // number of masks (0, 1, or nBands)
      const Byte* pValidBytes,         // masks (size = nMasks * nRows * nCols)
      double maxZErr,                  // max coding error per pixel, defines the precision
      unsigned int& numBytes,          // size of outgoing Lerc blob
      const unsigned char* pUsesNoData,// if there are invalid values not marked by the mask, pass an array of size nBands, 1 - uses noData, 0 - not
      const double* noDataValues);     // same, pass an array of size nBands with noData value per band, or pass nullptr

    template<class T> static ErrCode EncodeTempl(
      const T* pData,                  // raw image data, row by row, band by band
      int version,                     // 2 = v2.2, 3 = v2.3, 4 = v2.4 5 = v2.5
      int nDepth,                      // number of values per pixel
      int nCols,                       // number of cols
      int nRows,                       // number of rows
      int nBands,                      // number of bands
      int nMasks,                      // number of masks (0, 1, or nBands)
      const Byte* pValidBytes,         // masks (size = nMasks * nRows * nCols)
      double maxZErr,                  // max coding error per pixel, defines the precision
      Byte* pBuffer,                   // buffer to write to, function will fail if buffer too small
      unsigned int numBytesBuffer,     // buffer size
      unsigned int& numBytesWritten,   // num bytes written to buffer
      const unsigned char* pUsesNoData,// if there are invalid values not marked by the mask, pass an array of size nBands, 1 - uses noData, 0 - not
      const double* noDataValues);     // same, pass an array of size nBands with noData value per band, or pass nullptr

    template<class T> static ErrCode DecodeTempl(
      T* pData,                        // outgoing data bands
      const Byte* pLercBlob,           // Lerc blob to decode
      unsigned int numBytesBlob,       // size of Lerc blob in bytes
      int nDepth,                      // number of values per pixel
      int nCols,                       // number of cols
      int nRows,                       // number of rows
      int nBands,                      // number of bands
      int nMasks,                      // number of masks (0, 1, or nBands)
      Byte* pValidBytes,               // masks (fails if not big enough to take the masks decoded, fills with 1 if all valid)
      unsigned char* pUsesNoData,      // pass an array of size nBands, 1 - band uses noData, 0 - not
      double* noDataValues);           // same, pass an array of size nBands to get the noData value per band, if any

  private:

    template<class T> static ErrCode EncodeInternal_v5(
      const T* pData,                  // raw image data, row by row, band by band
      int version,                     // 2 = v2.2, 3 = v2.3, 4 = v2.4, 5 = v2.5
      int nDepth,                      // number of values per pixel
      int nCols,                       // number of cols
      int nRows,                       // number of rows
      int nBands,                      // number of bands
      int nMasks,                      // number of masks (0, 1, or nBands)
      const Byte* pValidBytes,         // masks (size = nMasks * nRows * nCols)
      double maxZErr,                  // max coding error per pixel, defines the precision
      unsigned int& numBytes,          // size of outgoing Lerc blob
      Byte* pBuffer,                   // buffer to write to, function will fail if buffer too small
      unsigned int numBytesBuffer,     // buffer size
      unsigned int& numBytesWritten);  // num bytes written to buffer

    template<class T> static ErrCode EncodeInternal(
      const T* pData,                  // raw image data, row by row, band by band
      int version,                     // 6 = v2.6 (or -1 for current)
      int nDepth,                      // number of values per pixel
      int nCols,                       // number of cols
      int nRows,                       // number of rows
      int nBands,                      // number of bands
      int nMasks,                      // number of masks (0, 1, or nBands)
      const Byte* pValidBytes,         // masks (size = nMasks * nRows * nCols)
      double maxZErr,                  // max coding error per pixel, defines the precision
      unsigned int& numBytes,          // size of outgoing Lerc blob
      Byte* pBuffer,                   // buffer to write to, function will fail if buffer too small
      unsigned int numBytesBuffer,     // buffer size
      unsigned int& numBytesWritten,   // num bytes written to buffer
      const unsigned char* pUsesNoData,// if there are invalid values not marked by the mask, pass an array of size nBands, 1 - uses noData, 0 - not
      const double* noDataValues);     // same, pass an array of size nBands with noData value per band, or pass nullptr

#ifdef HAVE_LERC1_DECODE
    template<class T> static bool Convert(const CntZImage& zImg, T* arr, Byte* pByteMask, bool bMustFillMask);
#endif
    template<class T> static ErrCode ConvertToDoubleTempl(const T* pDataIn, size_t nDataValues, double* pDataOut);

    template<class T> static ErrCode CheckForNaN(const T* arr, int nDepth, int nCols, int nRows, const Byte* pByteMask);

    template<class T> static bool ReplaceNaNValues(std::vector<T>& dataBuffer, std::vector<Byte>& maskBuffer, int nDepth, int nCols, int nRows);

    template<class T> static bool Resize(std::vector<T>& buffer, size_t nElem);

    static bool Convert(const Byte* pByteMask, int nCols, int nRows, BitMask& bitMask);
    static bool Convert(const BitMask& bitMask, Byte* pByteMask);
    static bool MasksDiffer(const Byte* p0, const Byte* p1, size_t n);

    static ErrCode GetRanges(const Byte* pLercBlob, unsigned int numBytesBlob, int iBand,
      const struct Lerc2::HeaderInfo& lerc2Info, double* pMins, double* pMaxs, size_t nElem);

    template<class T>
    static bool RemapNoData(T* data, const BitMask& bitMask, const struct Lerc2::HeaderInfo& lerc2Info);

    template<class T>
    static bool DecodeAndCompareToInput(const Byte* pLercBlob, size_t blobSize, double maxZErr, Lerc2& lerc2Verify,
      const T* pData, const Byte* pByteMask, const T* pDataOrig, const Byte* pByteMaskOrig,
      bool bInputHasNoData, double origNoDataA, bool bModifiedMask);

    template<class T>
    static bool GetTypeRange(const T, std::pair<double, double>& range);

    template<class T>
    inline static bool IsInt(T z) { return(z == (T)floor((double)z + 0.5)); };

    template<class T>
    static ErrCode FilterNoData(std::vector<T>& dataBuffer, std::vector<Byte>& maskBuffer, int nDepth, int nCols, int nRows,
      double& maxZError, bool bPassNoDataValue, double& noDataValue, bool& bModifiedMask, bool& bNeedNoData);

    template<class T>
    static ErrCode FilterNoDataAndNaN(std::vector<T>& dataBuffer, std::vector<Byte>& maskBuffer, int nDepth, int nCols, int nRows,
      double& maxZError, bool bPassNoDataValue, double& noDataValue, bool& bModifiedMask, bool& bNeedNoData, bool& bIsFltDblAllInt);

    template<class T>
    static bool FindNewNoDataBelowValidMin(double minVal, double maxZErr, bool bAllInt, double lowIntLimit, T& newNoDataVal);
  };
NAMESPACE_LERC_END
