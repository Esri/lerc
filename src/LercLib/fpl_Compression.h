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
