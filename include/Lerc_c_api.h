/*
Copyright 2016 Esri

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

#ifndef LERC_API_INCLUDE_GUARD
#define LERC_API_INCLUDE_GUARD

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MSC_VER)
  #define LERCDLL_API __declspec(dllexport)
#elif __GNUC__ >= 4
  #define LERCDLL_API __attribute__((visibility("default")))
#else
  #define LERCDLL_API
#endif


  //! C-API for LERC library
  //! Unless specified:
  //!   - all output buffers must have been allocated by caller. 
  //!   - Input buffers do not need to outlive Lerc function call (Lerc will make internal copies if needed).
  
  typedef unsigned int lerc_status;

  //! Compute the buffer size in bytes required to hold the compressed input tile. Optional. 
  //! You can call lerc_encode(...) directly as long as the output buffer is large enough. 

  //! Order of raw input data is top left corner to lower right corner, row by row. This for each band. 
  //! Data type is  { char = 0, uchar = 1, short = 2, ushort = 3, int = 4, uint = 5, float = 6, double = 7 }, see Lerc_types.h .
  //! maxZErr is the max compression error tolerated per pixel.
  //! The tile of valid bytes is optional. Zero means all pixels are valid. 
  //! If not all pixels are valid, set those bytes to 0, valid pixel bytes to 1. 
  //! Size of the valid / invalid byte image is nCols x nRows. 

  LERCDLL_API
  lerc_status lerc_computeCompressedSize(
    const void* pData,                   // raw image data, row by row, band by band
    unsigned int dataType,
    int nCols, int nRows, int nBands,    // number of columns, rows, bands
    const unsigned char* pValidBytes,    // 0 if all pixels are valid; 1 byte per pixel (1 = valid, 0 = invalid)
    double maxZErr,                      // max coding error per pixel, defines the precision
    unsigned int* numBytes);             // size of outgoing Lerc blob


  //! Encode the input data into a compressed Lerc blob.

  LERCDLL_API
  lerc_status lerc_encode(
    const void* pData,
    unsigned int dataType,
    int nCols, int nRows, int nBands,
    const unsigned char* pValidBytes,
    double maxZErr,
    unsigned char* pOutBuffer,           // buffer to write to, function fails if buffer too small
    unsigned int outBufferSize,          // size of output buffer
    unsigned int* nBytesWritten);        // number of bytes written to output buffer


  //! Call this to get info about the compressed Lerc blob. Optional. 
  //! Info returned in infoArray is { version, dataType, nCols, nRows, nBands, nValidPixels, blobSize }, see Lerc_types.h .
  //! Info returned in dataRangeArray is { zMin, zMax, maxZErrorUsed }, see Lerc_types.h .
  //! If more than 1 band this is over all bands. 

  // Remark on param blobSize. Usually it is known, either the file size of the blob written to disk, 
  // or the size of the blob transmitted. It should be accurate for 2 reasons:
  // _ function finds out how many single band Lerc blobs are concatenated
  // _ function checks for truncated file or blob
  // It is OK to pass blobSize too large as long as there is no other (valid) Lerc blob following next.
  // If in doubt, check the code in Lerc::GetLercInfo(...) for the exact logic. 

  LERCDLL_API
  lerc_status lerc_getBlobInfo(const unsigned char* pLercBlob, unsigned int blobSize, 
    unsigned int* infoArray, double* dataRangeArray, int infoArraySize = 7, int dataRangeArraySize = 3);


  //! Decode the compressed Lerc blob into a raw data array.
  //! The data array must have been allocated to size (nCols * nRows * nBands * sizeof(dataType)).
  //! The valid bytes array, if not 0, must have been allocated to size (nCols * nRows). 

  LERCDLL_API
  lerc_status lerc_decode(
    const unsigned char* pLercBlob,     // Lerc blob to decode
    unsigned int blobSize,              // blob size in bytes
    unsigned char* pValidBytes,         // gets filled if not 0, even if all valid
    int nCols, int nRows, int nBands,   // number of columns, rows, bands
    unsigned int dataType,              // data type of outgoing array
    void* pData);                       // outgoing data array


#ifdef __cplusplus
}
#endif

#endif //LERC_API_INCLUDE_GUARD
