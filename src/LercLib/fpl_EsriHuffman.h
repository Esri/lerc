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
 * Original coding 2021 Yuriy Yakimenko
*/

#ifndef FPL_ESRI_HUFFMAN_H
#define FPL_ESRI_HUFFMAN_H

#include <cstddef>
#include "Defines.h"

NAMESPACE_LERC_START

class fpl_EsriHuffman
{
public:
  static int EncodeHuffman(const char* input, size_t input_len, unsigned char** ppByte, bool use_rle);

  static bool DecodeHuffman(const unsigned char* inBytes, const size_t inCount, size_t& nBytesRemainingInOut, unsigned char** output);

  static int getCompressedSize(const char* input, size_t input_len);
};

NAMESPACE_LERC_END
#endif
