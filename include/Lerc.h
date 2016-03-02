/*
Copyright 2015 Esri

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

#include <vector>
#include <cstring>
#include "BitMask.h"

namespace LercNS
{
  class CntZImage;

  class Lerc
  {
  public:
    LERCDLL_API Lerc() {};
    LERCDLL_API ~Lerc() {};

    // data types supported by Lerc
    enum DataType { DT_Char, DT_Byte, DT_Short, DT_UShort, DT_Int, DT_UInt, DT_Float, DT_Double, DT_Undefined };

    // all functions are provided in 2 flavors
    // - using void pointers to the image data, can be called on a Lerc lib or dll
    // - data templated, can be called if compiled together


    // Encode

    // if more than 1 band, the outgoing Lerc blob has the single band Lerc blobs concatenated

    // computes the number of bytes needed to allocate the buffer, accurate to the byte;
    // does not encode the image data, but uses statistics and formulas to compute the buffer size needed;
    // this function is optional, you can also use a buffer large enough to call Encode() directly,
    // or, if encoding a batch of same width / height tiles, call this function once, double the buffer size, and
    // then just call Encode() on all tiles;

    LERCDLL_API
    bool ComputeBufferSize(const void* pData,  // raw image data, row by row, band by band
      DataType dt,
      int nCols, int nRows, int nBands,    // number of columns, rows, bands
      const BitMask* pBitMask,             // 0 if all pixels are valid
      double maxZError,                    // max coding error per pixel, defines the precision
      size_t& numBytesNeeded) const;       // size of outgoing Lerc blob

    // encodes or compresses the image data into the buffer

    LERCDLL_API
    bool Encode(const void* pData,         // raw image data, row by row, band by band
      DataType dt,
      int nCols, int nRows, int nBands,    // number of columns, rows, bands
      const BitMask* pBitMask,             // 0 if all pixels are valid
      double maxZError,                    // max coding error per pixel, defines the precision
      Byte* pBuffer,                       // buffer to write to, function will fail if buffer too small
      size_t numBytesBuffer,               // buffer size
      size_t& numBytesWritten) const;      // num bytes written to buffer


    // Decode

    struct LercInfo
    {
      int version,        // Lerc version number: 2, 1, or 0 (0 for old Lerc1)
        nCols,            // number of columns
        nRows,            // number of rows
        numValidPixel,    // number of valid pixels
        nBands,           // number of bands
        blobSize;         // total blob size in bytes
      DataType dt;        // data type (float for old Lerc1)
      double zMin,        // min pixel value, over all bands
        zMax,             // max pixel value, over all bands
        maxZError;        // maxZError used for encoding

      void RawInit()  { memset(this, 0, sizeof(struct LercInfo)); }
    };

    // again, this function is optional;
    // call it on a Lerc blob to get the above struct returned, from this the data arrays
    // can be constructed before calling Decode();
    // same as above, for a batch of Lerc blobs of the same kind, you can call this function on 
    // the first blob, get the info, and on the other Lerc blobs just call Decode();
    // this function is very fast on (newer) Lerc2 blobs as it only reads the blob headers;

    LERCDLL_API
    bool GetLercInfo(const Byte* pLercBlob, size_t numBytesBlob, struct LercInfo& lercInfo) const;

    // setup outgoing arrays accordingly, then call Decode()

    LERCDLL_API
    bool Decode(const Byte* pLercBlob,    // Lerc blob to be decoded
      size_t numBytesBlob,                // size in bytes
      BitMask* pBitMask,                  // gets filled if not 0, even if all valid
      int nCols, int nRows, int nBands,   // number of columns, rows, bands
      DataType dt,                        // data type of outgoing array
      void* pData) const;                 // outgoing data bands


    // same as functions above, but data templated instead of using void pointers

    template<class T>
    bool ComputeBufferSizeTempl(const T* pData,    // raw image data, row by row, band by band
      int nCols, int nRows, int nBands,    // number of columns, rows, bands
      const BitMask* pBitMask,             // 0 if all pixels are valid
      double maxZError,                    // max coding error per pixel, defines the precision
      size_t& numBytesNeeded) const;       // size of outgoing Lerc blob

    template<class T>
    bool EncodeTempl(const T* pData,       // raw image data, row by row, band by band
      int nCols, int nRows, int nBands,    // number of columns, rows, bands
      const BitMask* pBitMask,             // 0 if all pixels are valid
      double maxZError,                    // max coding error per pixel, defines the precision
      Byte* pBuffer,                       // buffer to write to, function will fail if buffer too small
      size_t numBytesBuffer,               // buffer size
      size_t& numBytesWritten) const;      // num bytes written to buffer

    // alternative encode function, allocates the Lerc blob inside

    template<class T>
    bool EncodeTempl(const T* pData,       // raw image data, row by row, band by band
      int nCols, int nRows, int nBands,    // number of columns, rows, bands
      const BitMask* pBitMask,             // 0 if all pixels are valid
      double maxZError,                    // max coding error per pixel, defines the precision
      Byte** ppLercBlob,                   // outgoing Lerc blob, call delete[] when done
      size_t& numBytesBlob) const;         // size of outgoing Lerc blob

    template<class T>
    bool DecodeTempl(T* pData,             // outgoing data bands
      const Byte* pLercBlob,               // Lerc blob to decode
      size_t numBytesBlob,                 // size of Lerc blob in bytes
      int nCols, int nRows, int nBands,    // number of columns, rows, bands
      BitMask* pBitMask) const;            // gets filled if not 0, even if all valid

  private:
    void FreeBuffer(std::vector<Byte*>& bufferVec) const;
    template<class T> bool Convert(const CntZImage& zImg, T* arr, BitMask* pBitMask) const;
  };
}    // namespace LercNS
