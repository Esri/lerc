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

#include "fpl_Compression.h"
#include "fpl_EsriHuffman.h"
#include <assert.h>
#include <cmath>
#include <cstring>

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

#ifdef _DEBUG
        //if (ret < 0)
        //  fprintf(stderr, "Huffman failed. Input size: %zd. Code: %d\n", size, ret);
#endif

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
