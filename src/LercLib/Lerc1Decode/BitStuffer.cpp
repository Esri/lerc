/*
Copyright 2015 - 2022 Esri

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

#include <cstring>
#include "BitStuffer.h"

using namespace std;
using namespace LercNS;

// -------------------------------------------------------------------------- ;

bool BitStuffer::read(const Byte** ppByte, vector<unsigned int>& dataVec) const
{
  if (!ppByte)
    return false;

  Byte numBitsByte = **ppByte;
  (*ppByte)++;

  int bits67 = numBitsByte >> 6;
  int n = (bits67 == 0) ? 4 : 3 - bits67;

  numBitsByte &= 63;    // bits 0-5;

  unsigned int numElements = 0;
  if (!readUInt(ppByte, numElements, n))
    return false;

  if (numBitsByte >= 32)
    return false;

  int numBits = numBitsByte;
  unsigned int numUInts = (numElements * numBits + 31) / 32;
  dataVec.resize(numElements, 0);    // init with 0

  if (numUInts > 0)    // numBits can be 0
  {
    m_tmpBitStuffVec.resize(numUInts);
    m_tmpBitStuffVec[numUInts - 1] = 0;    // set last uint to 0

    unsigned int nBytesToCopy = (numElements * numBits + 7) / 8;
    memcpy(&m_tmpBitStuffVec[0], *ppByte, nBytesToCopy);

    unsigned int* arr = &m_tmpBitStuffVec[0];
    unsigned int* srcPtr = arr;
    for (unsigned int i = 0; i < numUInts; i++)
    {
      SWAP_4(*srcPtr);
      srcPtr++;
    }

    unsigned int* pLastULong = &m_tmpBitStuffVec[numUInts - 1];
    unsigned int n = numTailBytesNotNeeded(numElements, numBits);
    while (n--)
      *pLastULong <<= 8;

    // do the un-stuffing
    srcPtr = arr;
    unsigned int* dstPtr = &dataVec[0];
    int bitPos = 0;

    for (unsigned int i = 0; i < numElements; i++)
    {
      if (32 - bitPos >= numBits)
      {
        unsigned int val;
        memcpy(&val, srcPtr, sizeof(unsigned int));
        unsigned int n = val << bitPos;
        *dstPtr++ = n >> (32 - numBits);

        bitPos += numBits;
        if (bitPos == 32)    // shift >= 32 is undefined
        {
          bitPos = 0;
          srcPtr++;
        }
      }
      else
      {
        unsigned int val;
        memcpy(&val, srcPtr, sizeof(unsigned int));
        srcPtr++;
        unsigned int n = val << bitPos;
        *dstPtr = n >> (32 - numBits);
        bitPos -= (32 - numBits);
        memcpy(&val, srcPtr, sizeof(unsigned int));
        *dstPtr++ |= val >> (32 - bitPos);
      }
    }

    *ppByte += nBytesToCopy;
  }

  return true;
}

// -------------------------------------------------------------------------- ;
// -------------------------------------------------------------------------- ;

bool BitStuffer::readUInt(const Byte** ppByte, unsigned int& k, int numBytes)
{
  const Byte* ptr = *ppByte;

  if (numBytes == 1)
  {
    k = *ptr;
  }
  else if (numBytes == 2)
  {
    unsigned short s;
    memcpy(&s, ptr, sizeof(unsigned short));
    SWAP_2(s);
    k = s;
  }
  else if (numBytes == 4)
  {
    memcpy(&k, ptr, sizeof(unsigned int));
    SWAP_4(k);
  }
  else
    return false;

  *ppByte = ptr + numBytes;
  return true;
}

// -------------------------------------------------------------------------- ;

unsigned int BitStuffer::numTailBytesNotNeeded(unsigned int numElem, int numBits)
{
  int numBitsTail = (numElem * numBits) & 31;
  int numBytesTail = (numBitsTail + 7) >> 3;
  return (numBytesTail > 0) ? 4 - numBytesTail : 0;
}

// -------------------------------------------------------------------------- ;

