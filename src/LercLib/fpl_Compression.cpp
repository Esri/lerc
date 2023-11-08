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

#include "fpl_Compression.h"
#include "fpl_EsriHuffman.h"
#include <assert.h>
#include <cmath>
#include <cstring>
#include <climits>

USING_NAMESPACE_LERC

static const bool use_esri_huffman = true;
static const bool use_rle = true;

size_t fpl_Compression::extract_buffer(const char* data, size_t size, size_t uncompressed_size, char** output)
{
    if (use_esri_huffman)
    {
        size_t expected_size = uncompressed_size;

        bool ret = fpl_EsriHuffman::DecodeHuffman((const unsigned char*)data, size, expected_size, (unsigned char**)output);

        //if (!ret)
        //  printf("input size %zu, expected out %zu\n", size, uncompressed_size);

        assert(ret);
        return uncompressed_size;
    }

    assert(0);
    return 0;
}

size_t fpl_Compression::compress_buffer(const char* data, size_t size, char** output, bool fast)
{
    if (use_esri_huffman)
    {
        if (fast && !output)
        {
            //  return getCompressedSize (data, size);

            return getEntropySize((unsigned char*)data, size);  // this is approximate but fastest
        }

        assert(size > 0);

        int ret = fpl_EsriHuffman::EncodeHuffman(data, size, (unsigned char**)output, use_rle);

        if (ret < 0)
        {
#ifdef _DEBUG
          //  fprintf(stderr, "Huffman failed. Input size: %zd. Code: %d\n", size, ret);
#endif
          return (size_t)UINT_MAX + 1;
        }

        return ret;
    }

    return 0;
}

long fpl_Compression::getEntropySize(const unsigned char* ptr, const size_t size)
{
  unsigned long table[256];

  memset(table, 0, sizeof(table));

  int total_count = 0;

  for (size_t i = 0; i < size; i += PRIME_MULT)
  {
    table[ptr[i]]++;
    total_count++;
  }

  double total_bits = 0;

  for (size_t i = 0; i < 256; i++)
  {
    if (table[i] == 0) continue;

    double p = (double)total_count / table[i];

    double bits = log2(p);

    total_bits += (bits * table[i]);
  }

  return (long)((total_bits + 7) / 8);
}
