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
