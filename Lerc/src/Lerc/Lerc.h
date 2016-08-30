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

#include <cstring>
#include <vector>
#include "../Include/Lerc_types.h"
#include "../Common/BitMask.h"
#include "../Lerc2/Lerc2.h"

namespace LercNS
{
  class CntZImage;

  class Lerc
  {
  public:
    Lerc() {};
    ~Lerc() {};

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

    static ErrCode ComputeCompressedSize(const void* pData,   // raw image data, row by row, band by band
      DataType dt,
      int nCols, int nRows, int nBands,            // number of columns, rows, bands
      const BitMask* pBitMask,                     // 0 if all pixels are valid
      double maxZErr,                              // max coding error per pixel, defines the precision
      unsigned int& numBytesNeeded);               // size of outgoing Lerc blob

    // encodes or compresses the image data into the buffer

    static ErrCode Encode(const void* pData,       // params same as above unless noted otherwise
      DataType dt,
      int nCols, int nRows, int nBands,
      const BitMask* pBitMask,
      double maxZErr,
      Byte* pBuffer,                               // buffer to write to, function fails if buffer too small
      unsigned int numBytesBuffer,                 // buffer size
      unsigned int& numBytesWritten);              // num bytes written to buffer


    // Decode

    struct LercInfo
    {
      int version,        // Lerc version number (0 for old Lerc1)
        nCols,            // number of columns
        nRows,            // number of rows
        numValidPixel,    // number of valid pixels
        nBands,           // number of bands
        blobSize;         // total blob size in bytes
      DataType dt;        // data type (float only for old Lerc1)
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

    // Remark on param numBytesBlob. Usually it is known, either the file size of the blob written to disk, 
    // or the size of the blob transmitted. It should be accurate for 2 reasons:
    // _ function finds out how many single band Lerc blobs are concatenated
    // _ function checks for truncated file or blob
    // It is OK to pass numBytesBlob too large as long as there is no other (valid) Lerc blob following next.
    // If in doubt, check the code in Lerc::GetLercInfo(...) for the exact logic. 

    static ErrCode GetLercInfo(const Byte* pLercBlob,       // Lerc blob to decode
                               unsigned int numBytesBlob,   // size of Lerc blob in bytes
                               struct LercInfo& lercInfo);

    // setup outgoing arrays accordingly, then call Decode()

    static ErrCode Decode(const Byte* pLercBlob,     // Lerc blob to decode
      unsigned int numBytesBlob,                     // size of Lerc blob in bytes
      BitMask* pBitMask,                             // gets filled if not 0, even if all valid
      int nCols, int nRows, int nBands,              // number of columns, rows, bands
      DataType dt,                                   // data type of outgoing array
      void* pData);                                  // outgoing data bands


    // same as functions above, but data templated instead of using void pointers

    template<class T> static
      ErrCode ComputeCompressedSizeTempl(const T* pData,   // raw image data, row by row, band by band
      int nCols, int nRows, int nBands,              // number of columns, rows, bands
      const BitMask* pBitMask,                       // 0 if all pixels are valid
      double maxZErr,                                // max coding error per pixel, defines the precision
      unsigned int& numBytes);                       // size of outgoing Lerc blob

    template<class T> static
    ErrCode EncodeTempl(const T* pData,              // raw image data, row by row, band by band
      int nCols, int nRows, int nBands,              // number of columns, rows, bands
      const BitMask* pBitMask,                       // 0 if all pixels are valid
      double maxZErr,                                // max coding error per pixel, defines the precision
      Byte* pBuffer,                                 // buffer to write to, function will fail if buffer too small
      unsigned int numBytesBuffer,                   // buffer size
      unsigned int& numBytesWritten);                // num bytes written to buffer

    // alternative encode function, allocates the Lerc blob inside

    template<class T> static
    ErrCode EncodeTempl(const T* pData,              // raw image data, row by row, band by band
      int nCols, int nRows, int nBands,              // number of columns, rows, bands
      const BitMask* pBitMask,                       // 0 if all pixels are valid
      double maxZErr,                                // max coding error per pixel, defines the precision
      Byte** ppLercBlob,                             // outgoing Lerc blob, call delete[] when done
      unsigned int& numBytesBlob);                   // size of outgoing Lerc blob

    template<class T> static
    ErrCode DecodeTempl(T* pData,                    // outgoing data bands
      const Byte* pLercBlob,                         // Lerc blob to decode
      unsigned int numBytesBlob,                     // size of Lerc blob in bytes
      int nCols, int nRows, int nBands,              // number of columns, rows, bands
      BitMask* pBitMask);                            // gets filled if not 0, even if all valid

  private:
    static void FreeBuffer(std::vector<Byte*>& bufferVec);
    template<class T> static bool Convert(const CntZImage& zImg, T* arr, BitMask* pBitMask);
  };
}    // namespace LercNS
