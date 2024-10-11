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

#include "fpl_Lerc2Ext.h"
#include "fpl_Compression.h"
#include <assert.h>
#include <cmath>
#include <cstring>
#include <stdint.h>
#include <algorithm>

USING_NAMESPACE_LERC

void lerc_assert(bool v)
{
  if (v == false)
    throw "Assertion failed";
}

template<typename T>
size_t getMinIndex(const T* array, size_t size)
{
  T min_value = array[0];
  size_t ret = 0;

  for (size_t i = 1; i < size; i++)
  {
    if (array[i] < min_value)
    {
      min_value = array[i];
      ret = i;
    }
  }

  return ret;
}

struct TestBlock
{
  long top, height;
};

static void generateListOfTestBlocks(const int width, const int height, std::vector <TestBlock>& blocks)
{
  size_t size = (size_t)width * height;

  lerc_assert(size > 0);

  const int block_target_size = 8 * 1024;

  double t = (round)((double)size / block_target_size);

  int count = (int)round(sqrt(t + 1));   // Always at least 1. We want count to grow but not linearly as size grows.
                                         // A more conservative scheme would be to use log not sqrt.

  // when block size is too wide (>100K) we may need to consider taking a "narrower part"

  int block_height = block_target_size / width;

  if (block_height < 4) block_height = 4;

  while ((count * block_height > height) && (count > 1)) count--;

  float top_margin = (float)((height - count * block_height) / (2.0 * count));

  float delta = 2.0f * top_margin + block_height;

  for (int i = 0; i < count; i++)
  {
    TestBlock tb;

    tb.top = (long)(top_margin + delta * i);
    tb.height = block_height;
    if (tb.top < 0) tb.top = 0;
    if (tb.top + tb.height > height) tb.height = height - tb.top;

    if (tb.height > 0)
      blocks.push_back(tb);
  }
}

static void setDerivativePrime(unsigned char* data, size_t size)
{
  int off = (int)size - 1;

  off = PRIME_MULT * (off / PRIME_MULT);

  unsigned char* ptr = data + off;
  //for (int i = (int)off; i >= 1; i -= PRIME_MULT)
  while (ptr >= data + 1)
  {
    *ptr -= *(ptr - 1);
    ptr -= PRIME_MULT;
  }
}

static void setDerivative(unsigned char* data, size_t size, const int level)
{
  if (level == 0) return;

  for (int l = 1; l <= level; l++)
  {
    unsigned char* ptr = data + size - 1;
    for (int i = (int)size - 1; i >= l; i--)
    {
      *ptr -= *(ptr - 1);
      ptr--;
    }
  }
}

unsigned char* restoreSequence(unsigned char* data, size_t size, const int level, bool make_copy)
{
  assert(size > 0);

  unsigned char* copy = NULL;

  if (make_copy)
  {
    copy = (unsigned char*)malloc(size);

    if (!copy)
    {
      //perror("Out of memory.");
      return NULL;
    }

    memcpy(copy, data, size);
  }
  else
    copy = data;

  if (level == 0)
    return copy;

  for (int l = level; l > 0; l--)
  {
    unsigned char* ptr = copy + l;

    for (int i = l; i < (int)size; i++)
    {
      *ptr += *(ptr - 1);
      ptr++;
    }
  }

  return copy;
}

static size_t testBlocksSize(std::vector <TestBlock>& blocks, const UnitType unit_type, const void* _data, const long raster_width, bool test_first_byte_delta)
{
  size_t ret = 0;

  const uint8_t* data = (const uint8_t*)_data;

  const size_t unit_size = UnitTypes::size(unit_type);

  for (size_t blk = 0; blk < blocks.size(); blk++)
  {
    TestBlock& tb = blocks[blk];

    size_t start = unit_size * tb.top * raster_width;
    size_t length = tb.height * raster_width;

   // input_len += length * unit_size; //sizeof(uint32_t);

    char* plane_buffer = (char*)malloc(length);

    assert(plane_buffer);

    for (int byte = 0; byte < (int)unit_size; byte++)
    {
      char* ptr = (char*)(data + start);

      ptr += byte;

      for (size_t i = 0; i < length; i++)
      {
        plane_buffer[i] = *ptr;

        ptr += unit_size;
      }

      size_t plane_encoded = fpl_Compression::compress_buffer((const char*)plane_buffer, length, NULL, true);

      // test with delta 1:

      if (test_first_byte_delta)
      {
        //  setDerivative ((unsigned char *)plane_buffer, length, 1);

        setDerivativePrime((unsigned char*)plane_buffer, length);

        size_t plane_encoded2 = fpl_Compression::compress_buffer((const char*)plane_buffer, length, NULL, true);

        ret += (std::min)(plane_encoded, plane_encoded2);
      }
      else
      {
        ret += plane_encoded;
      }
    }

    free(plane_buffer);
  }

  return ret;
}


/////////////////////////////////////////////////////////////////////////////////////////////////////////


int getBestLevel2(const unsigned char* ptr, size_t size, const int max_delta_order)
{
  std::vector<std::pair<size_t, int> > snippets;

  const unsigned int targetSampleSize = 1024 * 8;

  double t = round((double)size / targetSampleSize);

  int count = (int)round(sqrt(t + 1));   // Always at least 1. We want count to grow but not linearly as size grows.
                                      // A more conservative scheme would be to use log not sqrt.

  while (count * targetSampleSize > size && (count > 0)) count--;

  float top_margin = (float)(((int)size - count * targetSampleSize) / (2.0 * count));

  float delta = 2.0f * top_margin + targetSampleSize;

  for (int i = 0; i < count; i++)
  {
    long start = (long)(top_margin + delta * i);
    int len = targetSampleSize;

    if (start < 0)
      start = 0;
    if (start + len > (int)size)
      len = (int)size - start;

    assert(len > 0);

    if (len > 0)
      snippets.push_back(std::make_pair(start, len));
  }

  unsigned char* copy = (unsigned char*)malloc(size);

  if (!copy)
    return 0; // return 0 as fall-back "default" delta.

  memcpy(copy, ptr, size);

  size_t best_comp = 0;

  int ret = 0;

  for (int l = 0; l <= max_delta_order; l++)
  {
    if (l > 0)
    {
      for (size_t s = 0; s < snippets.size(); s++)
      {
        size_t start = snippets[s].first;
        int len = snippets[s].second;

        for (int i = (int)start + len - 1; i >= (int)start + l; i--)
        {
          copy[i] -= copy[i - 1];
        }
      }
    }

    size_t comp = 0;

    for (size_t i = 0; i < snippets.size(); i++)
    {
      size_t start = snippets[i].first;
      int len = snippets[i].second;

      comp += fpl_Compression::compress_buffer((const char*)copy + start, len, NULL, true);
    }

    if (comp < best_comp || l == 0)
    {
      best_comp = comp;
      ret = l;
    }
    else
    {
      break; // if deteriorating, stop.
    }
  }

  free(copy);

  return ret;
}

///////////////////////////////////////////////////////////////////////////////////////////////

int getBestLevel(const unsigned char* ptr, size_t size, const int max_delta_order)
{
  if (max_delta_order == 0)
    return 0;

  return getBestLevel2(ptr, size, max_delta_order);
}

///////////////////////////////////////////////////////////////////////////////////////////////

LosslessFPCompression::~LosslessFPCompression()
{
  delete m_data_slice;
}

void LosslessFPCompression::selectInitialLinearOrCrossDelta(const UnitType unit_type, void* pData, const int iWidth,
  const int iHeight, int& initial_delta, bool& use_cross, bool test_first_byte_delta, size_t* stats)
{
  std::vector <TestBlock> blocks;

  size_t local_stats[3] = { 0 };

  if (stats == NULL)
  {
    stats = local_stats;
  }

  generateListOfTestBlocks(iWidth, iHeight, blocks);

  size_t est = 0;

  // unchanged block:
  est = testBlocksSize(blocks, unit_type, pData, iWidth, test_first_byte_delta);

  if (stats) { stats[0] += est; }

  // linear part:

  UnitTypes::setBlockDerivative(unit_type, pData, iWidth, iHeight, 1, 1);

  est = testBlocksSize(blocks, unit_type, pData, iWidth, test_first_byte_delta);

  if (stats) { stats[1] += est; }

  UnitTypes::setCrossDerivative(unit_type, pData, iWidth, iHeight, 2, 2);

  est = testBlocksSize(blocks, unit_type, pData, iWidth, test_first_byte_delta);

  if (stats) { stats[2] += est; }

  size_t min_index = getMinIndex<size_t>(stats, 3);

  if (min_index == 2)
  {
    use_cross = true;
    initial_delta = 2;
  }
  else if (min_index == 1)
  {
    use_cross = false;
    initial_delta = 1;
  }
  else
  {
    use_cross = false;
    initial_delta = 0;
  }
}

int LosslessFPCompression::compressedLength() const
{
  int ret = 1; // predictor code

  for (auto b : m_data_slice->m_buffers)
  {
    ret += b->compressed_size;
    ret += 6; // 2 bytes plus compr. size. We can use 1 byte in place of 2 by packing level and byte index.
  }

  return ret;
}

bool LosslessFPCompression::EncodeHuffmanFlt(unsigned char ** ppByte)
{
  memcpy(*ppByte, &(m_data_slice->m_predictor_code), sizeof(m_data_slice->m_predictor_code));
  *ppByte += sizeof(m_data_slice->m_predictor_code);

  for (auto b : m_data_slice->m_buffers)
  {
      memcpy(*ppByte, &(b->byte_index), sizeof(b->byte_index));
      *ppByte += sizeof(b->byte_index);
      memcpy(*ppByte, &(b->best_level), sizeof(b->best_level));
      *ppByte += sizeof(b->best_level);
      memcpy(*ppByte, &(b->compressed_size), sizeof(b->compressed_size));
      *ppByte += sizeof(b->compressed_size);
      memcpy(*ppByte, b->compressed, b->compressed_size);
      *ppByte += b->compressed_size;
  }

  for (auto v : m_data_slice->m_buffers)
  {
    delete v;
  }

  m_data_slice->m_buffers.clear();

  return true;
}

bool LosslessFPCompression::ComputeHuffmanCodesFlt(const void* input, bool bIsDouble,
                int iCols, int iRows, int iDepth)
{
  if (iDepth == 1)
  {
    if (m_data_slice && !m_data_slice->m_buffers.empty())
    {
      // we decided not to write compressed output last time.
      // in this case, old compressed content must be removed.
      for (auto v : m_data_slice->m_buffers)
      {
        delete v;
      }

      m_data_slice->m_buffers.clear();
    }
    return ComputeHuffmanCodesFltSlice (input, bIsDouble, iCols, iRows);
  }
  else
  {
    return ComputeHuffmanCodesFltSlice(input, bIsDouble, iDepth, iCols * iRows);
  }
}

bool LosslessFPCompression::ComputeHuffmanCodesFltSlice (const void* pInput, bool bIsDouble, int iCols, int iRows)
{
  // 1. copy input; when input is 32-bit floats, move bits of copied input values.
  // 2. copy to temp buffer.
  // 3. perform selectInitialLinearOrCrossDelta on temp copy; deallocate temp copy.
  // 4. apply best predictor.
  // 5. compress individual byte planes while applying additional dynamic delta (bestLevel).
  // 6. save compessed byte planes in m_buffers and predictor in m_predictor_code.
  // 7. deallocate copied input.

  UnitType unit_type = bIsDouble ? UNIT_TYPE_DOUBLE : UNIT_TYPE_FLOAT;

  int max_byte_delta = MAX_DELTA;   // in pactice it will rarely be more than 3.
                                    // I observed values greater than 3 only with double data.
                                    // see getMaxByteDelta() call below.

  size_t size = (size_t)iCols * iRows;

  size_t unit_size = UnitTypes::size(unit_type);
  size_t block_size = size;

  uint8_t* block_values = (uint8_t*)malloc(block_size * unit_size);

  memcpy(block_values, pInput, block_size * unit_size);

  if (unit_type == UNIT_TYPE_FLOAT)
    UnitTypes::doFloatTransform((uint32_t*)block_values, size);

  size_t stats1[3] = { 0 };

  int block_width = iCols, block_height = iRows;

  int dummy_delta = 0;
  bool dummy_cross = false;
  bool test_first_byte_delta = true;

  uint8_t* copy = (uint8_t*)malloc(block_size * unit_size);

  if (!copy)
  {
    //perror("Out of memory.");
    free(block_values);
    return false;
  }

  memcpy(copy, block_values, unit_size * block_size);

  selectInitialLinearOrCrossDelta(unit_type, copy, block_width, block_height, dummy_delta, dummy_cross, test_first_byte_delta, stats1);

  free(copy);

  size_t min_index = getMinIndex<size_t>(stats1, 3);

  PredictorType predictor = PREDICTOR_NONE;

  if (min_index < 2)
  {
    if (min_index == 1) predictor = PREDICTOR_DELTA1;
  }
  else
  {
    if (min_index == 2) predictor = PREDICTOR_ROWS_COLS;
  }

  int w = block_width, h = block_height;

  if (predictor == PREDICTOR_ROWS_COLS)
  {
    if (predictor == PREDICTOR_ROWS_COLS)
      UnitTypes::setCrossDerivative(unit_type, block_values, w, h, 2);
  }
  else
  {
    int delta = 0;

    if (predictor == PREDICTOR_DELTA1) delta = 1;

    UnitTypes::setBlockDerivative(unit_type, block_values, w, h, delta);
  }

  int max_delta = Predictor::getMaxByteDelta(predictor);

  if (max_byte_delta >= 0 && max_byte_delta < max_delta)
    max_delta = max_byte_delta;

  unsigned char* block_buff = (unsigned char*)malloc(block_size); // size is the same for all byte planes

  if (!block_buff)
  {
    //perror("Out of memory.");
    free(block_values);
    return false;
  }

  if (!m_data_slice)
    m_data_slice = new compressedDataSlice();

  for (int byte = 0; byte < (int)unit_size; byte++)
  {
    size_t block_index = 0;

    size_t block_shift = 0;

    block_index = byte;

    size_t len = (size_t)h * w;

    while (block_shift < len)
    {
      block_buff[block_shift] = block_values[block_index];

      block_index += unit_size;

      block_shift++;
    }

    int bestLevel = getBestLevel(block_buff, block_size, max_delta);

    setDerivative(block_buff, block_size, bestLevel);

    char* compressed = NULL;

    size_t ret = fpl_Compression::compress_buffer((const char*)block_buff, block_size, &compressed);

    if (ret > UINT32_MAX) // cannot store compressed size in 32 bits. should not happen.
    {
      free(block_buff);
      free(block_values);

      return false;
    }

    // Down the road, we also can store compression method code (other than default Huffman)
    // in upper top 6 bits, since max predicor value is 2.

    m_data_slice->m_predictor_code = Predictor::getCode(predictor);

    {
      outBlockBuffer* ob = new outBlockBuffer;
      ob->compressed = compressed;
      ob->compressed_size = (uint32_t)ret;
      ob->byte_index = (unsigned char)byte;
      ob->best_level = (unsigned char)bestLevel;

      m_data_slice->m_buffers.push_back(ob);
    }
  }

  free(block_buff);
  free(block_values);

  return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////

bool restoreCrossBytes(std::vector<std::pair<int, char*> >& output_buffers, const size_t input_in_bytes,
  const size_t cols, const size_t rows, const PredictorType predictor,
  //FILE* output,
  const UnitType unit_type, uint8_t** output_block)
{
  lerc_assert(predictor == PREDICTOR_NONE || predictor == PREDICTOR_ROWS_COLS);

  size_t unit_size = output_buffers.size();

  lerc_assert(unit_size == UnitTypes::size(unit_type));

  const int delta = Predictor::getIntDelta(predictor);

  size_t block_size = cols * rows;

  assert(input_in_bytes == block_size);

  uint8_t* data_buffer = (uint8_t*)malloc(block_size * unit_size);

  if (NULL == data_buffer)
  {
    //perror("Out of memory.");
    return false;
  }

  size_t offset = 0;

  for (size_t i = 0; i < block_size; i++)
  {
    for (size_t byte = 0; byte < unit_size; byte++)
    {
      int idx = output_buffers[byte].first;
      data_buffer[idx + offset] = output_buffers[byte].second[i];
    }

    offset += unit_size;
  }

  UnitTypes::restoreCrossBytes(delta, data_buffer, cols, rows, unit_type);

  if (unit_type == UNIT_TYPE_FLOAT)
  {
    UnitTypes::undoFloatTransform((uint32_t*)data_buffer, block_size);
  }

  if (output_block)
  {
    *output_block = data_buffer;
  }
  else
  {
    free(data_buffer); // Coverity warning fix. Currently, execution never gets here.
  }

  return true;
}

bool restoreByteOrder(std::vector<std::pair<int, char*> >& output_buffers,
  const size_t cols, const size_t rows, const PredictorType predictor, //FILE* output,
  const UnitType unit_type, uint8_t** output_block)
{
  lerc_assert(predictor == PREDICTOR_NONE || predictor == PREDICTOR_DELTA1);

  size_t unit_size = output_buffers.size();

  lerc_assert(unit_size == UnitTypes::size(unit_type));

  const int delta = Predictor::getIntDelta(predictor);

  const size_t block_size = cols * rows;

  uint8_t* data_buffer = (uint8_t*)malloc(block_size * unit_size);

  if (NULL == data_buffer)
  {
    //perror("Out of memory.");
    return false;
  }

  size_t offset = 0;

  for (size_t i = 0; i < block_size; i++)
  {
    for (size_t byte = 0; byte < unit_size; byte++)
    {
      int idx = output_buffers[byte].first;
      data_buffer[idx + offset] = output_buffers[byte].second[i];
    }

    offset += unit_size;
  }

  UnitTypes::restoreBlockSequence(delta, data_buffer, cols, rows, unit_type);

  if (unit_type == UNIT_TYPE_FLOAT)
  {
    UnitTypes::undoFloatTransform((uint32_t*)data_buffer, block_size);
  }

  if (output_block)
  {
    *output_block = data_buffer;
  }
  else
  {
    free(data_buffer); // Coverity warning fix. Currently, execution never gets here.
  }

  return true;
}

//////////////////////////////////////////////////////////////////////////////////////

bool LosslessFPCompression::DecodeHuffmanFlt(const unsigned char** ppByte, size_t& nBytesRemainingInOut,
  void* pData, bool bIsDouble, int iWidth, int iHeight, int iDepth)
{
  if (iDepth == 1)
  {
    return DecodeHuffmanFltSlice (ppByte, nBytesRemainingInOut, pData, bIsDouble, iWidth, iHeight);
  }
  else
  {
    return DecodeHuffmanFltSlice(ppByte, nBytesRemainingInOut, pData, bIsDouble, iDepth, iWidth * iHeight);
  }
}

bool LosslessFPCompression::DecodeHuffmanFltSlice (const unsigned char** ppByte, size_t& nBytesRemainingInOut,
          void * pData, bool bIsDouble, int iWidth, int iHeight)
{
  unsigned char* ptr = (unsigned char *)(*ppByte);

  UnitType unit_type = bIsDouble ? UNIT_TYPE_DOUBLE : UNIT_TYPE_FLOAT;

  size_t bytes = UnitTypes::size(unit_type);

  size_t expected_size = ((size_t)iWidth * iHeight);

  std::vector<std::pair<int, char*> >output_buffers;

  unsigned char pred_code = 0;

  memcpy(&pred_code, ptr, sizeof(pred_code));

  if (pred_code > 2) // down the road, upper 6 bits can contain compression method.
                     // we support only Huffman in this version for now.
  {
    return false;
  }

  ptr += sizeof(pred_code);

  nBytesRemainingInOut -= sizeof(pred_code);

  for (size_t byte = 0; byte < bytes; byte++)
  {
    unsigned char byte_index = 0, best_level = 0;

    uint32_t compressed_size = 0;

    if (nBytesRemainingInOut < 6)
      return false;

    memcpy(&byte_index, ptr, sizeof(byte_index));

    if (byte_index >= bytes)
      return false;

    ptr += sizeof(byte_index);
    nBytesRemainingInOut -= sizeof(byte_index);

    memcpy(&best_level, ptr, sizeof(best_level));
    ptr += sizeof(best_level);
    nBytesRemainingInOut -= sizeof(best_level);

    if (best_level > MAX_DELTA)
      return false;

    memcpy(&compressed_size, ptr, sizeof(compressed_size));
    ptr += sizeof(compressed_size);
    nBytesRemainingInOut -= sizeof(compressed_size);

    if (nBytesRemainingInOut < compressed_size)
      return false;

    unsigned char* compressed = (unsigned char *)malloc(compressed_size);

    if (!compressed)
      return false;

    memcpy(compressed, ptr, compressed_size);
    ptr += compressed_size;

    nBytesRemainingInOut -= compressed_size;

    char* uncompressed = NULL;

    size_t extracted_size = fpl_Compression::extract_buffer((const char *)compressed, compressed_size, expected_size, &uncompressed);

    lerc_assert(expected_size == extracted_size);

    free(compressed);
    compressed = NULL;

    int byte_delta = best_level;

    unsigned char* restored = restoreSequence((unsigned char*)uncompressed, extracted_size, byte_delta, false);

    output_buffers.push_back(std::make_pair(byte_index, (char*)restored));
  }

 // nBytesRemainingInOut -= (ptr - *ppByte); => done above.
  *ppByte = ptr;

  PredictorType predictor = Predictor::getType(pred_code);

  const bool use_cross_delta = (predictor == PREDICTOR_ROWS_COLS);

  uint8_t* output_block_data = NULL;

  bool ret = (predictor != PREDICTOR_UNKNOWN);

  if (ret)
  {
    if (use_cross_delta)
    {
      ret = restoreCrossBytes(output_buffers, expected_size, iWidth, iHeight, predictor,
        //NULL,
        unit_type,
        &output_block_data);
    }
    else
    {
      ret = restoreByteOrder(output_buffers, iWidth, iHeight, predictor,
        //NULL,
        unit_type,
        &output_block_data);
    }
  }

  for (size_t i = 0; i < output_buffers.size(); i++)
  {
    free(output_buffers[i].second);
  }

  output_buffers.clear();

  if (output_block_data) // ret is already set to false if memory was not allocated.
  {
    memcpy(pData, output_block_data, bytes * iWidth * iHeight);

    free(output_block_data);
  }

  return (ret);
}
