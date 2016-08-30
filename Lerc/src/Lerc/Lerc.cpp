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

Contributors:  Thomas Maurer
*/

#include "Lerc.h"
#include "../Lerc2/Lerc2.h"
#include "../Lerc1Decode/CntZImage.h"

using namespace std;
using namespace LercNS;

// -------------------------------------------------------------------------- ;

bool Lerc::ComputeBufferSize(const void* pData,    // raw image data, row by row, band by band
  DataType dt,
  int nCols, int nRows, int nBands,
  const BitMask* pBitMask,          // 0 if all pixels are valid
  double maxZError,                 // max coding error per pixel, or precision
  size_t& numBytesNeeded) const     // size of outgoing Lerc blob
{
  switch (dt)
  {
  case DT_Char:    return ComputeBufferSizeTempl((const char*)pData, nCols, nRows, nBands, pBitMask, maxZError, numBytesNeeded);
  case DT_Byte:    return ComputeBufferSizeTempl((const Byte*)pData, nCols, nRows, nBands, pBitMask, maxZError, numBytesNeeded);
  case DT_Short:   return ComputeBufferSizeTempl((const short*)pData, nCols, nRows, nBands, pBitMask, maxZError, numBytesNeeded);
  case DT_UShort:  return ComputeBufferSizeTempl((const unsigned short*)pData, nCols, nRows, nBands, pBitMask, maxZError, numBytesNeeded);
  case DT_Int:     return ComputeBufferSizeTempl((const int*)pData, nCols, nRows, nBands, pBitMask, maxZError, numBytesNeeded);
  case DT_UInt:    return ComputeBufferSizeTempl((const unsigned int*)pData, nCols, nRows, nBands, pBitMask, maxZError, numBytesNeeded);
  case DT_Float:   return ComputeBufferSizeTempl((const float*)pData, nCols, nRows, nBands, pBitMask, maxZError, numBytesNeeded);
  case DT_Double:  return ComputeBufferSizeTempl((const double*)pData, nCols, nRows, nBands, pBitMask, maxZError, numBytesNeeded);

  default:
    return false;
  }

  return true;
}

// -------------------------------------------------------------------------- ;

bool Lerc::Encode(const void* pData,    // raw image data, row by row, band by band
  DataType dt,
  int nCols, int nRows, int nBands,
  const BitMask* pBitMask,          // 0 if all pixels are valid
  double maxZError,                 // max coding error per pixel, or precision
  Byte* pBuffer,                    // buffer to write to, function will fail if buffer too small
  size_t numBytesBuffer,            // buffer size
  size_t& numBytesWritten) const    // num bytes written to buffer
{
  switch (dt)
  {
  case DT_Char:    return EncodeTempl((const char*)pData, nCols, nRows, nBands, pBitMask, maxZError, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_Byte:    return EncodeTempl((const Byte*)pData, nCols, nRows, nBands, pBitMask, maxZError, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_Short:   return EncodeTempl((const short*)pData, nCols, nRows, nBands, pBitMask, maxZError, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_UShort:  return EncodeTempl((const unsigned short*)pData, nCols, nRows, nBands, pBitMask, maxZError, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_Int:     return EncodeTempl((const int*)pData, nCols, nRows, nBands, pBitMask, maxZError, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_UInt:    return EncodeTempl((const unsigned int*)pData, nCols, nRows, nBands, pBitMask, maxZError, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_Float:   return EncodeTempl((const float*)pData, nCols, nRows, nBands, pBitMask, maxZError, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_Double:  return EncodeTempl((const double*)pData, nCols, nRows, nBands, pBitMask, maxZError, pBuffer, numBytesBuffer, numBytesWritten);

  default:
    return false;
  }

  return true;
}

// -------------------------------------------------------------------------- ;

bool Lerc::GetLercInfo(const Byte* pLercBlob, size_t numBytesBlob, struct LercInfo& lercInfo) const
{
  lercInfo.RawInit();

  // first try Lerc2

  Lerc2 lerc2;
  unsigned int minNumBytesHeader = lerc2.ComputeMinNumBytesNeededToReadHeader();

  struct Lerc2::HeaderInfo lerc2Info;
  if (minNumBytesHeader <= numBytesBlob && lerc2.GetHeaderInfo(pLercBlob, lerc2Info))
  {
    lercInfo.version = lerc2Info.version;
    lercInfo.nCols = lerc2Info.nCols;
    lercInfo.nRows = lerc2Info.nRows;
    lercInfo.numValidPixel = lerc2Info.numValidPixel;
    lercInfo.nBands = 1;
    lercInfo.blobSize = lerc2Info.blobSize;
    lercInfo.dt = (DataType)lerc2Info.dt;
    lercInfo.zMin = lerc2Info.zMin;
    lercInfo.zMax = lerc2Info.zMax;
    lercInfo.maxZError = lerc2Info.maxZError;

    if (lercInfo.blobSize > (int)numBytesBlob)    // truncated blob, we won't be able to read this band
      return false;

    while (lercInfo.blobSize + minNumBytesHeader < numBytesBlob)    // means there could be another band
    {
      struct Lerc2::HeaderInfo hdInfo;
      if (!lerc2.GetHeaderInfo(pLercBlob + lercInfo.blobSize, hdInfo))
        return true;    // no other band, we are done

      if (hdInfo.nCols != lercInfo.nCols
        || hdInfo.nRows != lercInfo.nRows
        || hdInfo.numValidPixel != lercInfo.numValidPixel
        || (int)hdInfo.dt != (int)lercInfo.dt
        || hdInfo.maxZError != lercInfo.maxZError)
      {
        return false;
      }

      lercInfo.blobSize += hdInfo.blobSize;

      if (lercInfo.blobSize > (int)numBytesBlob)    // truncated blob, we won't be able to read this band
        return false;

      lercInfo.nBands++;
      lercInfo.zMin = min(lercInfo.zMin, hdInfo.zMin);
      lercInfo.zMax = max(lercInfo.zMax, hdInfo.zMax);
    }

    return true;
  }

  // then try Lerc1
  unsigned int numBytesHeader = CntZImage::computeNumBytesNeededToReadHeader();
  Byte* pByte = const_cast<Byte*>(pLercBlob);

  CntZImage cntZImg;
  if (numBytesHeader <= numBytesBlob && cntZImg.read(&pByte, 1e12, true))    // read just the header
  {
    size_t nBytesRead = pByte - pLercBlob;
    size_t nBytesNeeded = 10 + 4 * sizeof(int) + 1 * sizeof(double);

    if (nBytesRead < nBytesNeeded)
      return false;

    Byte* ptr = const_cast<Byte*>(pLercBlob);
    ptr += 10 + 2 * sizeof(int);

    int height = *((const int*)ptr);  ptr += sizeof(int);
    int width  = *((const int*)ptr);  ptr += sizeof(int);
    double maxZErrorInFile = *((const double*)ptr);

    if (height > 20000 || width > 20000)    // guard against bogus numbers
      return false;

    lercInfo.nCols = width;
    lercInfo.nRows = height;
    lercInfo.dt = Lerc::DT_Float;
    lercInfo.maxZError = maxZErrorInFile;

    Byte* pByte = const_cast<Byte*>(pLercBlob);
    bool onlyZPart = false;

    while (lercInfo.blobSize + numBytesHeader < numBytesBlob)    // means there could be another band
    {
      if (!cntZImg.read(&pByte, 1e12, false, onlyZPart))
        return (lercInfo.nBands > 0);    // no other band, we are done

      onlyZPart = true;

      lercInfo.nBands++;
      lercInfo.blobSize = (int)(pByte - pLercBlob);

      // now that we have decoded it, we can go the extra mile and collect some extra info
      int numValidPixels = 0;
      float zMin =  FLT_MAX;
      float zMax = -FLT_MAX;

      for (int i = 0; i < height; i++)
      {
        for (int j = 0; j < width; j++)
          if (cntZImg(i, j).cnt > 0)
          {
            numValidPixels++;
            float z = cntZImg(i, j).z;
            zMax = max(zMax, z);
            zMin = min(zMin, z);
          }
      }

      lercInfo.numValidPixel = numValidPixels;
      lercInfo.zMin = min(lercInfo.zMin, zMin);
      lercInfo.zMax = max(lercInfo.zMax, zMax);
    }

    return true;
  }

  return false;
}

// -------------------------------------------------------------------------- ;

bool Lerc::Decode(const Byte* pLercBlob,
  size_t numBytesBlob,
  BitMask* pBitMask,    // gets filled if not 0, even if all valid
  int nCols, int nRows, int nBands,
  DataType dt,
  void* pData) const    // outgoing data bands
{
  switch (dt)
  {
  case DT_Char:    return DecodeTempl((char*)pData, pLercBlob, numBytesBlob, nCols, nRows, nBands, pBitMask);
  case DT_Byte:    return DecodeTempl((Byte*)pData, pLercBlob, numBytesBlob, nCols, nRows, nBands, pBitMask);
  case DT_Short:   return DecodeTempl((short*)pData, pLercBlob, numBytesBlob, nCols, nRows, nBands, pBitMask);
  case DT_UShort:  return DecodeTempl((unsigned short*)pData, pLercBlob, numBytesBlob, nCols, nRows, nBands, pBitMask);
  case DT_Int:     return DecodeTempl((int*)pData, pLercBlob, numBytesBlob, nCols, nRows, nBands, pBitMask);
  case DT_UInt:    return DecodeTempl((unsigned int*)pData, pLercBlob, numBytesBlob, nCols, nRows, nBands, pBitMask);
  case DT_Float:   return DecodeTempl((float*)pData, pLercBlob, numBytesBlob, nCols, nRows, nBands, pBitMask);
  case DT_Double:  return DecodeTempl((double*)pData, pLercBlob, numBytesBlob, nCols, nRows, nBands, pBitMask);

  default:
    return false;
  }

  return true;
}

// -------------------------------------------------------------------------- ;
// -------------------------------------------------------------------------- ;

template<class T>
bool Lerc::ComputeBufferSizeTempl(const T* pData,    // raw image data, row by row, band by band
  int nCols, int nRows, int nBands,
  const BitMask* pBitMask,          // 0 if all pixels are valid
  double maxZError,                 // max coding error per pixel, or precision
  size_t& numBytesNeeded) const     // size of outgoing Lerc blob
{
  numBytesNeeded = 0;

  if (pBitMask && (pBitMask->GetHeight() != nRows || pBitMask->GetWidth() != nCols))
    return false;

  if (maxZError < 0)
    maxZError = 0;

  if (nBands < 1)
    return false;

  Lerc2 lerc2;
  bool rv = pBitMask ? lerc2.Set(*pBitMask) : lerc2.Set(nCols, nRows);
  if (!rv)
    return false;

  // loop over the bands
  for (int iBand = 0; iBand < nBands; iBand++)
  {
    bool encMsk = (iBand == 0);    // store bit mask with first band only
    unsigned int nBytes = lerc2.ComputeNumBytesNeededToWrite(pData + nCols * nRows * iBand, maxZError, encMsk);
    if (nBytes <= 0)
      return false;

    numBytesNeeded += nBytes;
  }

  return true;
}

// -------------------------------------------------------------------------- ;

template<class T>
bool Lerc::EncodeTempl(const T* pData,    // raw image data, row by row, band by band
  int nCols, int nRows, int nBands,
  const BitMask* pBitMask,         // 0 if all pixels are valid
  double maxZError,                // max coding error per pixel, or precision
  Byte* pBuffer,                   // buffer to write to, function will fail if buffer too small
  size_t numBytesBuffer,           // buffer size
  size_t& numBytesWritten) const   // num bytes written to buffer
{
  if (!pBuffer)
    return false;

  if (pBitMask && (pBitMask->GetHeight() != nRows || pBitMask->GetWidth() != nCols))
    return false;

  if (maxZError < 0)
    maxZError = 0;

  if (nBands < 1)
    return false;

  Lerc2 lerc2;
  bool rv = pBitMask ? lerc2.Set(*pBitMask) : lerc2.Set(nCols, nRows);
  if (!rv)
    return false;

  Byte* pByte = pBuffer;

  // loop over the bands, encode into array of single band Lerc blobs
  for (int iBand = 0; iBand < nBands; iBand++)
  {
    bool encMsk = (iBand == 0);    // store bit mask with first band only
    const T* arr = pData + nCols * nRows * iBand;

    unsigned int nBytes = lerc2.ComputeNumBytesNeededToWrite(arr, maxZError, encMsk);
    if (nBytes == 0)
      return false;

    unsigned int nBytesAlloc = nBytes;

    if ((size_t)(pByte - pBuffer) + nBytesAlloc > numBytesBuffer)    // check we have enough space left
      return false;

    if (!lerc2.Encode(arr, &pByte))
      return false;
  }

  numBytesWritten = (pByte - pBuffer);
  return true;
}

// -------------------------------------------------------------------------- ;

template<class T>
bool Lerc::EncodeTempl(const T* pData,    // raw image data, row by row, band by band
  int nCols, int nRows, int nBands,
  const BitMask* pBitMask,       // 0 if all pixels are valid
  double maxZError,              // max coding error per pixel, or precision
  Byte** ppLercBlob,             // outgoing Lerc blob, delete[] when done
  size_t& numBytesBlob) const    // size of outgoing Lerc blob
{
  if (!ppLercBlob)
    return false;

  if (pBitMask && (pBitMask->GetHeight() != nRows || pBitMask->GetWidth() != nCols))
    return false;

  if (maxZError < 0)
    maxZError = 0;

  if (nBands < 1)
    return false;

  size_t numBytesAllBands = 0;
  std::vector<size_t> numBytesPerBandVec(nBands, 0);
  std::vector<Byte*> bufferPerBandVec(nBands, 0);

  Lerc2 lerc2;
  bool rv = pBitMask ? lerc2.Set(*pBitMask) : lerc2.Set(nCols, nRows);
  if (!rv)
    return false;

  // loop over the bands, encode into array of single band Lerc blobs
  for (int iBand = 0; iBand < nBands; iBand++)
  {
    bool encMsk = (iBand == 0);    // store bit mask with first band only
    const T* arr = pData + nCols * nRows * iBand;

    unsigned int nBytes = lerc2.ComputeNumBytesNeededToWrite(arr, maxZError, encMsk);
    if (nBytes == 0)
    {
      FreeBuffer(bufferPerBandVec);
      return false;
    }

    unsigned int nBytesAlloc = nBytes;
    bufferPerBandVec[iBand] = new Byte[nBytesAlloc];
    Byte* pByte = bufferPerBandVec[iBand];

    if (!lerc2.Encode(arr, &pByte))
    {
      FreeBuffer(bufferPerBandVec);
      return false;
    }

    numBytesPerBandVec[iBand] = pByte - bufferPerBandVec[iBand];    // num bytes really used
    numBytesAllBands += numBytesPerBandVec[iBand];
  }

  numBytesBlob = numBytesAllBands;

  if (nBands == 1)
  {
    ppLercBlob = &bufferPerBandVec[0];
  }
  else
  {
    *ppLercBlob = new Byte[numBytesBlob];    // allocate
    Byte* pByte = *ppLercBlob;

    for (int iBand = 0; iBand < nBands; iBand++)
    {
      size_t len = numBytesPerBandVec[iBand];
      memcpy(pByte, bufferPerBandVec[iBand], len);
      pByte += len;
      delete[] bufferPerBandVec[iBand];
    }
  }

  return true;
}

// -------------------------------------------------------------------------- ;

template<class T>
bool Lerc::DecodeTempl(T* pData,    // outgoing data bands
  const Byte* pLercBlob,
  size_t numBytesBlob,
  int nCols, int nRows, int nBands,
  BitMask* pBitMask) const    // gets filled if not 0, even if all valid
{
  if (!pLercBlob || !pData)
    return false;

  const Byte* pByte = pLercBlob;
  Byte* pByte1 = const_cast<Byte*>(pLercBlob);

  Lerc2 lerc2;
  CntZImage zImg;

  for (int iBand = 0; iBand < nBands; iBand++)
  {
    // first try Lerc2
    unsigned int minNumBytesHeader = lerc2.ComputeMinNumBytesNeededToReadHeader();
    Lerc2::HeaderInfo hdInfo;
    if (((size_t)(pByte - pLercBlob) + minNumBytesHeader <= numBytesBlob) && lerc2.GetHeaderInfo(pByte, hdInfo))
    {
      if (hdInfo.nCols != nCols || hdInfo.nRows != nRows)
        return false;

      if ((pByte - pLercBlob) + (size_t)hdInfo.blobSize > numBytesBlob)
        return false;

      // init bitMask
      if (pBitMask && iBand == 0)
        pBitMask->SetSize(nCols, nRows);

      T* arr = pData + nCols * nRows * iBand;

      if (!lerc2.Decode(&pByte, arr, (pBitMask && iBand == 0) ? pBitMask->Bits() : 0))
        return false;
    }
    else    // then try Lerc1
    {
      unsigned int numBytesHeader = CntZImage::computeNumBytesNeededToReadHeader();

      if ((size_t)(pByte - pLercBlob) + numBytesHeader > numBytesBlob)
        return false;

      bool onlyZPart = iBand > 0;
      if (!zImg.read(&pByte1, 1e12, false, onlyZPart))
        return false;

      if (zImg.getWidth() != nCols || zImg.getHeight() != nRows)
        return false;

      T* arr = pData + nCols * nRows * iBand;

      if (!Convert(zImg, arr, pBitMask))
        return false;
    }
  }  // iBand

  return true;
}

// -------------------------------------------------------------------------- ;
// -------------------------------------------------------------------------- ;

void Lerc::FreeBuffer(std::vector<Byte*>& bufferVec) const
{
  size_t size = bufferVec.size();
  for (size_t k = 0; k < size; k++)
  {
    delete[] bufferVec[k];
    bufferVec[k] = 0;
  }
}

// -------------------------------------------------------------------------- ;

template<class T>
bool Lerc::Convert(const CntZImage& zImg, T* arr, BitMask* pBitMask) const
{
  if (!arr || !zImg.getSize())
    return false;

  const bool fltPnt = (typeid(*arr) == typeid(double)) || (typeid(*arr) == typeid(float));

  int h = zImg.getHeight();
  int w = zImg.getWidth();

  if (pBitMask)
  {
    pBitMask->SetSize(w, h);
    pBitMask->SetAllValid();
  }

  const CntZ* srcPtr = zImg.getData();
  T* dstPtr = arr;
  int num = w * h;
  for (int k = 0; k < num; k++)
  {
    if (srcPtr->cnt > 0)
      *dstPtr = fltPnt ? (T)srcPtr->z : (T)floor(srcPtr->z + 0.5);
    else if (pBitMask)
      pBitMask->SetInvalid(k);

    srcPtr++;
    dstPtr++;
  }

  return true;
}

// -------------------------------------------------------------------------- ;

