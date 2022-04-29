/*
COPYRIGHT 1995-2021 ESRI

TRADE SECRETS: ESRI PROPRIETARY AND CONFIDENTIAL
Unpublished material - all rights reserved under the
Copyright Laws of the United States.

For additional information, contact:
Environmental Systems Research Institute, Inc.
Attn: Contracts Dept
380 New York Street
Redlands, California, USA 92373

email: contracts@esri.com
*/

/* Analytical Raster Compression.
 * Original coding Yuriy Yakimenko
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
