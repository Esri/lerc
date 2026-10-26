/*
Copyright 2015 - 2026 Esri

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

#include <climits>
#include <cstring>
#include <algorithm>
#include "CntZImage.h"
#include "BitStuffer.h"
#include "../BitMask.h"
#include "../RLE.h"

using namespace std;
using namespace LercNS;

// -------------------------------------------------------------------------- ;

CntZImage::CntZImage()
{
  type_                    = CNT_Z;
  m_bDecoderCanIgnoreMask  = false;

  memset(&m_infoFromComputeNumBytes, 0, sizeof(m_infoFromComputeNumBytes));
};

// -------------------------------------------------------------------------- ;

bool CntZImage::resizeFill0(int width, int height)
{
  if (!resize(width, height))
    return false;

  memset(getData(), 0, sizeof(CntZ) * width * height);
  return true;
}

// -------------------------------------------------------------------------- ;

unsigned int CntZImage::computeNumBytesNeededToReadHeader(bool onlyZPart)
{
  CntZImage zImg;
  unsigned int cnt = (unsigned int)zImg.getTypeString().length();  // "CntZImage ", 10 bytes
  cnt += 4 * sizeof(int);       // version, type, width, height
  cnt += 1 * sizeof(double);    // maxZError
  if (!onlyZPart)
    cnt += 3 * sizeof(int) + sizeof(float);    // cnt part
  cnt += 3 * sizeof(int) + sizeof(float);    // z part
  cnt += 1;
  return cnt;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::read(const Byte** ppByte, const Byte* bArr_end, double maxZError, bool onlyHeader, bool onlyZPart)
{
  if (!ppByte || !*ppByte)
    return false;

  size_t len = getTypeString().length();

  if (*ppByte + len > bArr_end)
    return false;

  string typeStr(len, '0');
  memcpy(&typeStr[0], *ppByte, len);
  *ppByte += len;

  if (typeStr != getTypeString())
    return false;

  int version = 0, type = 0, width = 0, height = 0;
  double maxZErrorInFile = 0;

  const Byte* ptr = *ppByte;

  if (ptr + 4 * sizeof(int) + sizeof(double) > bArr_end)
    return false;

  memcpy(&version, ptr, sizeof(int));  ptr += sizeof(int);
  memcpy(&type,    ptr, sizeof(int));  ptr += sizeof(int);
  memcpy(&height,  ptr, sizeof(int));  ptr += sizeof(int);
  memcpy(&width,   ptr, sizeof(int));  ptr += sizeof(int);
  memcpy(&maxZErrorInFile, ptr, sizeof(double));  ptr += sizeof(double);

  *ppByte = ptr;

  SWAP_4(version);
  SWAP_4(type);
  SWAP_4(height);
  SWAP_4(width);
  SWAP_8(maxZErrorInFile);

  if (version != 11 || type != type_)
    return false;

  if (height < 0 || width < 0 || height > 40000 || width > 40000)  // guard against bogus numbers; size limitation for old Lerc1
    return false;

  if (sizeof(CntZ) * height * width > (size_t)INT_MAX)
    return false;

  if (maxZErrorInFile > maxZError)
    return false;

  if (onlyHeader)
    return true;

  if (!onlyZPart && !resizeFill0(width, height))    // init with (0,0), used below
    return false;

  m_bDecoderCanIgnoreMask = false;

  for (int iPart = 0; iPart < 2; iPart++)
  {
    bool zPart = iPart ? true : false;    // first cnt part, then z part

    if (!zPart && onlyZPart)
      continue;

    int numTilesVert = 0, numTilesHori = 0, numBytes = 0;
    float maxValInImg = 0;

    const Byte* ptr = *ppByte;

    if (ptr + 3 * sizeof(int) + sizeof(float) > bArr_end)
      return false;

    memcpy(&numTilesVert, ptr, sizeof(int));  ptr += sizeof(int);
    memcpy(&numTilesHori, ptr, sizeof(int));  ptr += sizeof(int);
    memcpy(&numBytes, ptr, sizeof(int));  ptr += sizeof(int);
    memcpy(&maxValInImg, ptr, sizeof(float));  ptr += sizeof(float);

    *ppByte = ptr;
    const Byte* bArr = ptr;

    SWAP_4(numTilesVert);
    SWAP_4(numTilesHori);
    SWAP_4(numBytes);
    SWAP_4(maxValInImg);

    if (numBytes < 0 || ptr + numBytes > bArr_end)
      return false;

    if (numTilesVert < 0 || numTilesVert > height_
      || numTilesHori < 0 || numTilesHori > width_
      || numBytes < 0 || numBytes > (1L << 30))    // limit to 1 GB
      return false;

    if (!zPart && numTilesVert == 0 && numTilesHori == 0)    // no tiling for this cnt part
    {
      if (numBytes == 0)    // cnt part is const
      {
        CntZ* dstPtr = getData();
        for (int i = 0; i < height_; i++)
          for (int j = 0; j < width_; j++)
          {
            dstPtr->cnt = maxValInImg;
            dstPtr++;
          }

        if (maxValInImg > 0)
          m_bDecoderCanIgnoreMask = true;
      }

      if (numBytes > 0)    // cnt part is binary mask, use fast RLE class
      {
        // decompress to bit mask
        BitMask bitMask(width_, height_);

        size_t bitMaskSize = bitMask.Size();
        if (bitMaskSize == 0)
          return false;

        size_t nBytesRemaining = (size_t)numBytes;  // checked against blob size above

        RLE rle;
        if (!rle.decompress(bArr, nBytesRemaining, (Byte*)bitMask.Bits(), bitMaskSize))
          return false;

        CntZ* dstPtr = getData();
        for (int k = 0, i = 0; i < height_; i++)
          for (int j = 0; j < width_; j++, k++, dstPtr++)
            dstPtr->cnt = bitMask.IsValid(k) ? 1.0f : 0.0f;
      }
    }
    else
    {
      size_t nBytesRemaining = (size_t)numBytes;
      if (!readTiles(zPart, maxZErrorInFile, numTilesVert, numTilesHori, maxValInImg, bArr, nBytesRemaining) || nBytesRemaining != 0)
        return false;
    }

    *ppByte += numBytes;
  }

  m_tmpDataVec.clear();
  return true;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::readTiles(bool zPart, double maxZErrorInFile, int numTilesVert, int numTilesHori,
  float maxValInImg, const Byte* bArr, size_t& nBytesRemaining)
{
  if (numTilesVert <= 0 || numTilesHori <= 0
    || numTilesVert > height_ || numTilesHori > width_)
    return false;

  const Byte* ptr = bArr;

  for (int iTile = 0; iTile <= numTilesVert; iTile++)
  {
    int tileH = height_ / numTilesVert;
    int i0 = iTile * tileH;
    if (iTile == numTilesVert)
      tileH = height_ % numTilesVert;

    if (tileH == 0)
      continue;

    for (int jTile = 0; jTile <= numTilesHori; jTile++)
    {
      int tileW = width_ / numTilesHori;
      int j0 = jTile * tileW;
      if (jTile == numTilesHori)
        tileW = width_ % numTilesHori;

      if (tileW == 0)
        continue;

      bool rv = zPart ? readZTile(  &ptr, nBytesRemaining, i0, i0 + tileH, j0, j0 + tileW, maxZErrorInFile, maxValInImg) :
                        readCntTile(&ptr, nBytesRemaining, i0, i0 + tileH, j0, j0 + tileW);

      if (!rv)
        return false;
    }
  }

  return true;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::readCntTile(const Byte** ppByte, size_t& nBytesRemaining, int i0, int i1, int j0, int j1)
{
  if (i0 >= i1 || j0 >= j1)
    return false;

  const size_t numPixel = (size_t)(i1 - i0) * (j1 - j0);

  if (nBytesRemaining < 1)
    return false;

  const Byte* ptr = *ppByte;
  Byte comprFlag = *ptr++;
  nBytesRemaining--;

  if (comprFlag == 2)    // entire tile is constant 0 (invalid)
  {                      // here we depend on resizeFill0()
    *ppByte = ptr;
    return true;
  }

  if (comprFlag == 3 || comprFlag == 4)    // entire tile is constant -1 (invalid) or 1 (valid)
  {
    CntZ cz1m = {-1, 0};
    CntZ cz1p = { 1, 0};
    CntZ cz1 = (comprFlag == 3) ? cz1m : cz1p;

    for (int i = i0; i < i1; i++)
    {
      CntZ* dstPtr = getData() + i * width_ + j0;
      for (int j = j0; j < j1; j++)
        *dstPtr++ = cz1;
    }

    *ppByte = ptr;
    return true;
  }

  if ((comprFlag & 63) > 4)
    return false;

  if (comprFlag == 0)
  {
    if (nBytesRemaining < numPixel * 4)
      return false;

    // read cnt's as flt arr uncompressed
    for (int i = i0; i < i1; i++)
    {
      CntZ* dstPtr = getData() + i * width_ + j0;
      for (int j = j0; j < j1; j++)
      {
        memcpy(&dstPtr->cnt, ptr, 4);  ptr += 4;
        SWAP_4(dstPtr->cnt);
        dstPtr++;
      }
    }

    nBytesRemaining -= numPixel * 4;
  }
  else
  {
    // read cnt's as int arr bit stuffed
    int bits67 = comprFlag >> 6;
    int n = (bits67 == 0) ? 4 : 3 - bits67;

    if (nBytesRemaining < (size_t)n)
      return false;

    float offset = 0;
    if (!readFlt(&ptr, offset, n))
      return false;

    nBytesRemaining -= n;

    BitStuffer bitStuffer;
    if (!bitStuffer.read(&ptr, nBytesRemaining, m_tmpDataVec, numPixel) || m_tmpDataVec.size() < numPixel)
      return false;

    unsigned int* srcPtr = &m_tmpDataVec[0];

    for (int i = i0; i < i1; i++)
    {
      CntZ* dstPtr = getData() + i * width_ + j0;
      for (int j = j0; j < j1; j++)
      {
        dstPtr->cnt = offset + (float)(*srcPtr++);
        dstPtr++;
      }
    }
  }

  *ppByte = ptr;
  return true;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::readZTile(const Byte** ppByte, size_t& nBytesRemaining, int i0, int i1, int j0, int j1, double maxZErrorInFile, float maxZInImg)
{
  const size_t maxNumPixel = (size_t)(i1 - i0) * (j1 - j0);

  if (nBytesRemaining < 1)
    return false;

  const Byte* ptr = *ppByte;
  Byte comprFlag = *ptr++;
  nBytesRemaining--;

  int bits67 = comprFlag >> 6;
  comprFlag &= 63;

  if (comprFlag == 2)    // entire zTile is constant 0 (if valid or invalid doesn't matter)
  {
    for (int i = i0; i < i1; i++)
    {
      CntZ* dstPtr = getData() + i * width_ + j0;
      for (int j = j0; j < j1; j++)
      {
        if (dstPtr->cnt > 0)
          dstPtr->z = 0;
        dstPtr++;
      }
    }

    *ppByte = ptr;
    return true;
  }

  if (comprFlag > 3)
    return false;

  if (comprFlag == 0)
  {
    // read z's as flt arr uncompressed
    for (int i = i0; i < i1; i++)
    {
      CntZ* dstPtr = getData() + i * width_ + j0;
      for (int j = j0; j < j1; j++)
      {
        if (dstPtr->cnt > 0)
        {
          if (nBytesRemaining < 4)
            return false;

          memcpy(&dstPtr->z, ptr, 4);  ptr += 4;
          SWAP_4(dstPtr->z);

          nBytesRemaining -= 4;
        }
        dstPtr++;
      }
    }
  }
  else
  {
    // read z's as int arr bit stuffed
    int n = (bits67 == 0) ? 4 : 3 - bits67;

    if (nBytesRemaining < (size_t)n)
      return false;

    float offset = 0;
    if (!readFlt(&ptr, offset, n))
      return false;

    nBytesRemaining -= n;

    if (comprFlag == 3)
    {
      for (int i = i0; i < i1; i++)
      {
        CntZ* dstPtr = getData() + i * width_ + j0;
        for (int j = j0; j < j1; j++)
        {
          if (dstPtr->cnt > 0)
            dstPtr->z = offset;
          dstPtr++;
        }
      }
    }
    else
    {
      BitStuffer bitStuffer;
      if (!bitStuffer.read(&ptr, nBytesRemaining, m_tmpDataVec, maxNumPixel))
        return false;

      double invScale = 2 * maxZErrorInFile;
      unsigned int* srcPtr = &m_tmpDataVec[0];
      size_t nElemLeft = m_tmpDataVec.size();

      if (m_bDecoderCanIgnoreMask)
      {
        if (nElemLeft != maxNumPixel)
          return false;

        for (int i = i0; i < i1; i++)
        {
          CntZ* dstPtr = getData() + i * width_ + j0;
          for (int j = j0; j < j1; j++)
          {
            float z = (float)(offset + *srcPtr++ * invScale);
            dstPtr->z = std::min(z, maxZInImg);    // make sure we stay in the orig range
            dstPtr++;
          }
        }
      }
      else
      {
        for (int i = i0; i < i1; i++)
        {
          CntZ* dstPtr = getData() + i * width_ + j0;
          for (int j = j0; j < j1; j++)
          {
            if (dstPtr->cnt > 0)
            {
              if (!nElemLeft--)
                return false;

              float z = (float)(offset + *srcPtr++ * invScale);
              dstPtr->z = std::min(z, maxZInImg);    // make sure we stay in the orig range
            }
            dstPtr++;
          }
        }
      }
    }
  }

  *ppByte = ptr;
  return true;
}

// -------------------------------------------------------------------------- ;

int CntZImage::numBytesFlt(float z)
{
  short s = (short)z;
  char c = (char)s;
  return ((float)c == z) ? 1 : ((float)s == z) ? 2 : 4;
}

// -------------------------------------------------------------------------- ;

bool CntZImage::readFlt(const Byte** ppByte, float& z, int numBytes)
{
  const Byte* ptr = *ppByte;

  if (numBytes == 1)
  {
    char c = *((char*)ptr);
    z = c;
  }
  else if (numBytes == 2)
  {
    short s;
    memcpy(&s, ptr, sizeof(short));
    SWAP_2(s);
    z = s;
  }
  else if (numBytes == 4)
  {
    memcpy(&z, ptr, sizeof(float));
    SWAP_4(z);
  }
  else
    return false;

  *ppByte = ptr + numBytes;
  return true;
}

// -------------------------------------------------------------------------- ;

