/*
Copyright 2021 - 2022 Esri

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

Analytical Raster Compression.
Original coding 2021 Yuriy Yakimenko
*/

#ifndef FPL_LERC2_EXT_H
#define FPL_LERC2_EXT_H

#include <cstdlib>
#include <vector>
#include "fpl_Predictor.h"
#include "fpl_UnitTypes.h"
#include "Defines.h"

NAMESPACE_LERC_START

class LosslessFPCompression
{
private:

  struct outBlockBuffer
  {
    char* compressed;
    uint32_t compressed_size; // assuming that tile single byte-plane compressed size will not exceed 32 bits.

    unsigned char byte_index; // we store byte index in case we decide to parallelize processing
                              // and byte planes end up not in strict order.
    unsigned char best_level;

    outBlockBuffer()
    {
      compressed = NULL;
      compressed_size = 0;
      byte_index = 0;
      best_level = 0;
    }

    outBlockBuffer(const outBlockBuffer&) = delete;             // disable copy constructor
    outBlockBuffer& operator=(const outBlockBuffer&) = delete;  // disable assignment

    ~outBlockBuffer()
    {
      free(compressed);
    }
  };

  struct compressedDataSlice
  {
    std::vector<outBlockBuffer*> m_buffers;
    unsigned char m_predictor_code;

    compressedDataSlice()
    {
      m_predictor_code = PREDICTOR_UNKNOWN;
    }

    compressedDataSlice(const compressedDataSlice&) = delete;             // disable copy constructor
    compressedDataSlice& operator=(const compressedDataSlice&) = delete;  // disable assignment

    ~compressedDataSlice()
    {
      for (auto b : m_buffers)
      {
        delete b;
      }

      m_buffers.clear();
    }
  };

  compressedDataSlice * m_data_slice;

  void selectInitialLinearOrCrossDelta(const UnitType type, void* pData, const int iWidth, const int iHeight, int& initial_delta, bool& use_cross, bool test_first_byte_delta, size_t* stats = NULL);

  bool ComputeHuffmanCodesFltSlice (const void* pInput, bool bIsDouble, int iCols, int iRows);

  static bool DecodeHuffmanFltSlice (const unsigned char** ppByte, size_t& nBytesRemainingInOut, void* pData,
    bool bIsDouble, int iCols, int iRows);


public:
  LosslessFPCompression() : m_data_slice (nullptr) { }
  ~LosslessFPCompression();

  bool ComputeHuffmanCodesFlt(const void * pInput, bool nIsDouble, int iCols, int iRows, int iDepth);
  int compressedLength() const;
  bool EncodeHuffmanFlt(unsigned char ** ppByte) ;

  static bool DecodeHuffmanFlt(const unsigned char** ppByte, size_t& nBytesRemainingInOut, void * pData,
    bool bIsDouble, int iCols, int iRows, int iDepth);
};

NAMESPACE_LERC_END
#endif
