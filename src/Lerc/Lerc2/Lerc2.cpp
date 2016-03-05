/*
Copyright 2015 Esri

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

Contributors:   Thomas Maurer
                Lucian Plesea (provided checksum code)
*/

#include "Lerc2.h"

using namespace LercNS;

// -------------------------------------------------------------------------- ;

Lerc2::Lerc2()
{
  Init();
}

// -------------------------------------------------------------------------- ;

Lerc2::Lerc2(int nCols, int nRows, const Byte* pMaskBits)
{
  Init();
  Set(nCols, nRows, pMaskBits);
}

// -------------------------------------------------------------------------- ;

void Lerc2::Init()
{
  m_currentVersion    = 3;    // 2: added Huffman coding to 8 bit types DT_Char, DT_Byte;
                              // 3: changed the bit stuffing to using a uint aligned buffer,
                              //    added Fletcher32 checksum
  m_microBlockSize    = 8;
  m_maxValToQuantize  = 0;
  m_encodeMask        = true;
  m_writeDataOneSweep = false;

  m_headerInfo.RawInit();
  m_headerInfo.version = m_currentVersion;
  m_headerInfo.microBlockSize = m_microBlockSize;
}

// -------------------------------------------------------------------------- ;

bool Lerc2::Set(int nCols, int nRows, const Byte* pMaskBits)
{
  if (!m_bitMask.SetSize(nCols, nRows))
    return false;

  if (pMaskBits)
  {
    memcpy(m_bitMask.Bits(), pMaskBits, m_bitMask.Size());
    m_headerInfo.numValidPixel = m_bitMask.CountValidBits();
  }
  else
  {
    m_headerInfo.numValidPixel = nCols * nRows;
    m_bitMask.SetAllValid();
  }

  m_headerInfo.nCols = nCols;
  m_headerInfo.nRows = nRows;

  return true;
}

// -------------------------------------------------------------------------- ;

bool Lerc2::Set(const BitMask& bitMask)
{
  m_bitMask = bitMask;

  m_headerInfo.numValidPixel = m_bitMask.CountValidBits();
  m_headerInfo.nCols = m_bitMask.GetWidth();
  m_headerInfo.nRows = m_bitMask.GetHeight();

  return true;
}

// -------------------------------------------------------------------------- ;

// if the Lerc2 header should ever shrink in size to less than below, then update it (very unlikely)

unsigned int Lerc2::ComputeMinNumBytesNeededToReadHeader() const
{
  unsigned int numBytes = (unsigned int)FileKey().length();
  numBytes += 7 * sizeof(int);
  numBytes += 3 * sizeof(double);
  return numBytes;
}

// -------------------------------------------------------------------------- ;

bool Lerc2::GetHeaderInfo(const Byte* pByte, struct HeaderInfo& headerInfo) const
{
  if (!pByte)
    return false;

  if (!IsLittleEndianSystem())
    return false;

  return ReadHeader(&pByte, headerInfo);
}

// -------------------------------------------------------------------------- ;
// -------------------------------------------------------------------------- ;

unsigned int Lerc2::ComputeNumBytesHeaderToWrite() const
{
  unsigned int numBytes = (unsigned int)FileKey().length();
  numBytes += 7 * sizeof(int);
  numBytes += 1 * sizeof(unsigned int);
  numBytes += 3 * sizeof(double);
  return numBytes;
}

// -------------------------------------------------------------------------- ;

bool Lerc2::WriteHeader(Byte** ppByte) const
{
  if (!ppByte)
    return false;

  std::string fileKey = FileKey();
  const HeaderInfo& hd = m_headerInfo;

  std::vector<int> intVec;
  intVec.push_back(hd.nRows);
  intVec.push_back(hd.nCols);
  intVec.push_back(hd.numValidPixel);
  intVec.push_back(hd.microBlockSize);
  intVec.push_back(hd.blobSize);
  intVec.push_back((int)hd.dt);

  std::vector<double> dblVec;
  dblVec.push_back(hd.maxZError);
  dblVec.push_back(hd.zMin);
  dblVec.push_back(hd.zMax);

  Byte* ptr = *ppByte;

  size_t len = fileKey.length();
  memcpy(ptr, fileKey.c_str(), len);
  ptr += len;

  memcpy(ptr, &m_currentVersion, sizeof(int));
  ptr += sizeof(int);

  unsigned int checksum = 0;
  memcpy(ptr, &checksum, sizeof(unsigned int));    // place holder to be filled by the real check sum later
  ptr += sizeof(unsigned int);

  len = intVec.size() * sizeof(int);
  memcpy(ptr, &intVec[0], len);
  ptr += len;

  len = dblVec.size() * sizeof(double);
  memcpy(ptr, &dblVec[0], len);
  ptr += len;

  *ppByte = ptr;
  return true;
}

// -------------------------------------------------------------------------- ;

bool Lerc2::ReadHeader(const Byte** ppByte, struct HeaderInfo& headerInfo) const
{
  if (!ppByte || !*ppByte)
    return false;

  const Byte* ptr = *ppByte;

  std::string fileKey = FileKey();
  HeaderInfo& hd = headerInfo;
  hd.RawInit();

  if (0 != memcmp(ptr, fileKey.c_str(), fileKey.length()))
    return false;

  ptr += fileKey.length();

  memcpy(&(hd.version), ptr, sizeof(int));
  ptr += sizeof(int);

  if (hd.version > m_currentVersion)    // this reader is outdated
    return false;

  if (hd.version >= 3)
  {
    memcpy(&(hd.checksum), ptr, sizeof(unsigned int));
    ptr += sizeof(unsigned int);
  }

  std::vector<int>  intVec(6, 0);
  std::vector<double> dblVec(3, 0);

  size_t len = sizeof(int) * intVec.size();
  memcpy(&intVec[0], ptr, len);
  ptr += len;

  len = sizeof(double) * dblVec.size();
  memcpy(&dblVec[0], ptr, len);
  ptr += len;

  hd.nRows          = intVec[0];
  hd.nCols          = intVec[1];
  hd.numValidPixel  = intVec[2];
  hd.microBlockSize = intVec[3];
  hd.blobSize       = intVec[4];
  hd.dt             = static_cast<DataType>(intVec[5]);

  hd.maxZError      = dblVec[0];
  hd.zMin           = dblVec[1];
  hd.zMax           = dblVec[2];

  *ppByte = ptr;
  return true;
}

// -------------------------------------------------------------------------- ;

bool Lerc2::WriteMask(Byte** ppByte) const
{
  if (!ppByte)
    return false;

  int numValid = m_headerInfo.numValidPixel;
  int numTotal = m_headerInfo.nCols * m_headerInfo.nRows;

  bool needMask = numValid > 0 && numValid < numTotal;

  Byte* ptr = *ppByte;

  if (needMask && m_encodeMask)
  {
    Byte* pArrRLE;
    size_t numBytesRLE;
    RLE rle;
    if (!rle.compress((const Byte*)m_bitMask.Bits(), m_bitMask.Size(), &pArrRLE, numBytesRLE, false))
      return false;

    int numBytesMask = (int)numBytesRLE;
    memcpy(ptr, &numBytesMask, sizeof(int));    // num bytes for compressed mask
    ptr += sizeof(int);
    memcpy(ptr, pArrRLE, numBytesRLE);
    ptr += numBytesRLE;

    delete[] pArrRLE;
  }
  else
  {
    memset(ptr, 0, sizeof(int));    // indicates no mask stored
    ptr += sizeof(int);
  }

  *ppByte = ptr;
  return true;
}

// -------------------------------------------------------------------------- ;

bool Lerc2::ReadMask(const Byte** ppByte)
{
  if (!ppByte)
    return false;

  int numValid = m_headerInfo.numValidPixel;
  int w = m_headerInfo.nCols;
  int h = m_headerInfo.nRows;

  const Byte* ptr = *ppByte;

  int numBytesMask;
  memcpy(&numBytesMask, ptr, sizeof(int));
  ptr += sizeof(int);

  if ((numValid == 0 || numValid == w * h) && (numBytesMask != 0))
    return false;

  if (!m_bitMask.SetSize(w, h))
    return false;

  if (numValid == 0)
    m_bitMask.SetAllInvalid();
  else if (numValid == w * h)
    m_bitMask.SetAllValid();
  else if (numBytesMask > 0)    // read it in
  {
    RLE rle;
    if (!rle.decompress(ptr, m_bitMask.Bits()))
      return false;

    ptr += numBytesMask;
  }
  // else use previous mask

  *ppByte = ptr;
  return true;
}

// -------------------------------------------------------------------------- ;

bool Lerc2::DoChecksOnEncode(Byte* pBlobBegin, Byte* pBlobEnd) const
{
  if ((size_t)(pBlobEnd - pBlobBegin) != (size_t)m_headerInfo.blobSize)
    return false;

  int blobSize = (int)(pBlobEnd - pBlobBegin);
  int nBytes = (int)(FileKey().length() + sizeof(int) + sizeof(unsigned int));    // start right after the checksum entry
  unsigned int checksum = ComputeChecksumFletcher32(pBlobBegin + nBytes, blobSize - nBytes);

  nBytes -= sizeof(unsigned int);
  memcpy(pBlobBegin + nBytes, &checksum, sizeof(unsigned int));

  return true;
}

// -------------------------------------------------------------------------- ;

// from  https://en.wikipedia.org/wiki/Fletcher's_checksum
// modified from ushorts to bytes (by Lucian Plesea)

unsigned int Lerc2::ComputeChecksumFletcher32(const Byte* pByte, int len) const
{
  unsigned int sum1 = 0xffff, sum2 = 0xffff;
  unsigned int words = len / 2;

  while (words)
  {
    unsigned int tlen = (words >= 359) ? 359 : words;
    words -= tlen;
    do {
      sum1 += (*pByte++ << 8);
      sum2 += sum1 += *pByte++;
    } while (--tlen);

    sum1 = (sum1 & 0xffff) + (sum1 >> 16);
    sum2 = (sum2 & 0xffff) + (sum2 >> 16);
  }

  // add the straggler byte if it exists
  if (len & 1)
    sum2 += sum1 += (*pByte << 8);

  // second reduction step to reduce sums to 16 bits
  sum1 = (sum1 & 0xffff) + (sum1 >> 16);
  sum2 = (sum2 & 0xffff) + (sum2 >> 16);

  return sum2 << 16 | sum1;
}

// -------------------------------------------------------------------------- ;

struct MyLessThanOp
{
  inline bool operator() (const std::pair<unsigned int, unsigned int>& p0,
                          const std::pair<unsigned int, unsigned int>& p1)  { return p0.first < p1.first; }
};

// -------------------------------------------------------------------------- ;

void Lerc2::SortQuantArray(const std::vector<unsigned int>& quantVec,
                           std::vector<std::pair<unsigned int, unsigned int> >& sortedQuantVec) const
{
  int numElem = (int)quantVec.size();
  sortedQuantVec.resize(numElem);

  for (int i = 0; i < numElem; i++)
    sortedQuantVec[i] = std::pair<unsigned int, unsigned int>(quantVec[i], i);

  std::sort(sortedQuantVec.begin(), sortedQuantVec.end(), MyLessThanOp());
}

// -------------------------------------------------------------------------- ;

