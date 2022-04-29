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

#ifndef FPL_COMPRESSION_H
#define FPL_COMPRESSION_H

#include <cstddef>
#include "Defines.h"

NAMESPACE_LERC_START

#define PRIME_MULT  7

class fpl_Compression
{
public:
  static size_t compress_buffer(const char* data, size_t size, char** output, bool fast = false);
  static size_t extract_buffer(const char* data, const size_t size, size_t uncompressed_size, char** output = 0);

private:
  static long getEntropySize(const unsigned char* ptr, const size_t size);
};

NAMESPACE_LERC_END
#endif
