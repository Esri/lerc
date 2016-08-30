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

ErrCode Lerc::ComputeCompressedSize(const void* pData,   // raw image data, row by row, band by band
  DataType dt,
  int nCols, int nRows, int nBands,
  const BitMask* pBitMask,                           // 0 if all pixels are valid
  double maxZErr,                                    // max coding error per pixel, or precision
  unsigned int& numBytesNeeded)                      // size of outgoing Lerc blob
{
  switch (dt)
  {
  case DT_Char:    return ComputeCompressedSizeTempl((const char*)pData, nCols, nRows, nBands, pBitMask, maxZErr, numBytesNeeded);
  case DT_Byte:    return ComputeCompressedSizeTempl((const Byte*)pData, nCols, nRows, nBands, pBitMask, maxZErr, numBytesNeeded);
  case DT_Short:   return ComputeCompressedSizeTempl((const short*)pData, nCols, nRows, nBands, pBitMask, maxZErr, numBytesNeeded);
  case DT_UShort:  return ComputeCompressedSizeTempl((const unsigned short*)pData, nCols, nRows, nBands, pBitMask, maxZErr, numBytesNeeded);
  case DT_Int:     return ComputeCompressedSizeTempl((const int*)pData, nCols, nRows, nBands, pBitMask, maxZErr, numBytesNeeded);
  case DT_UInt:    return ComputeCompressedSizeTempl((const unsigned int*)pData, nCols, nRows, nBands, pBitMask, maxZErr, numBytesNeeded);
  case DT_Float:   return ComputeCompressedSizeTempl((const float*)pData, nCols, nRows, nBands, pBitMask, maxZErr, numBytesNeeded);
  case DT_Double:  return ComputeCompressedSizeTempl((const double*)pData, nCols, nRows, nBands, pBitMask, maxZErr, numBytesNeeded);

  default:
    return ErrCode::WrongParam;
  }

  return ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;

ErrCode Lerc::Encode(const void* pData,    // raw image data, row by row, band by band
  DataType dt,
  int nCols, int nRows, int nBands,
  const BitMask* pBitMask,                 // 0 if all pixels are valid
  double maxZErr,                          // max coding error per pixel, or precision
  Byte* pBuffer,                           // buffer to write to, function will fail if buffer too small
  unsigned int numBytesBuffer,             // buffer size
  unsigned int& numBytesWritten)           // num bytes written to buffer
{
  switch (dt)
  {
  case DT_Char:    return EncodeTempl((const char*)pData, nCols, nRows, nBands, pBitMask, maxZErr, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_Byte:    return EncodeTempl((const Byte*)pData, nCols, nRows, nBands, pBitMask, maxZErr, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_Short:   return EncodeTempl((const short*)pData, nCols, nRows, nBands, pBitMask, maxZErr, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_UShort:  return EncodeTempl((const unsigned short*)pData, nCols, nRows, nBands, pBitMask, maxZErr, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_Int:     return EncodeTempl((const int*)pData, nCols, nRows, nBands, pBitMask, maxZErr, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_UInt:    return EncodeTempl((const unsigned int*)pData, nCols, nRows, nBands, pBitMask, maxZErr, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_Float:   return EncodeTempl((const float*)pData, nCols, nRows, nBands, pBitMask, maxZErr, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_Double:  return EncodeTempl((const double*)pData, nCols, nRows, nBands, pBitMask, maxZErr, pBuffer, numBytesBuffer, numBytesWritten);

  default:
    return ErrCode::WrongParam;
  }

  return ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;

ErrCode Lerc::GetLercInfo(const Byte* pLercBlob, unsigned int numBytesBlob, struct LercInfo& lercInfo)
{
  lercInfo.RawInit();

  // first try Lerc2

  unsigned int minNumBytesHeader = Lerc2::MinNumBytesNeededToReadHeader();

  struct Lerc2::HeaderInfo lerc2Info;
  if (minNumBytesHeader <= numBytesBlob && Lerc2::GetHeaderInfo(pLercBlob, lerc2Info))
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
      return ErrCode::BufferTooSmall;

    while (lercInfo.blobSize + minNumBytesHeader < numBytesBlob)    // means there could be another band
    {
      struct Lerc2::HeaderInfo hdInfo;
      if (!Lerc2::GetHeaderInfo(pLercBlob + lercInfo.blobSize, hdInfo))
        return ErrCode::Ok;    // no other band, we are done

      if (hdInfo.nCols != lercInfo.nCols
        || hdInfo.nRows != lercInfo.nRows
        || hdInfo.numValidPixel != lercInfo.numValidPixel
        || (int)hdInfo.dt != (int)lercInfo.dt
        || hdInfo.maxZError != lercInfo.maxZError)
      {
        return ErrCode::Failed;
      }

      lercInfo.blobSize += hdInfo.blobSize;

      if (lercInfo.blobSize > (int)numBytesBlob)    // truncated blob, we won't be able to read this band
        return ErrCode::BufferTooSmall;

      lercInfo.nBands++;
      lercInfo.zMin = min(lercInfo.zMin, hdInfo.zMin);
      lercInfo.zMax = max(lercInfo.zMax, hdInfo.zMax);
    }

    return ErrCode::Ok;
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
      return ErrCode::Failed;

    Byte* ptr = const_cast<Byte*>(pLercBlob);
    ptr += 10 + 2 * sizeof(int);

    int height = *((const int*)ptr);  ptr += sizeof(int);
    int width  = *((const int*)ptr);  ptr += sizeof(int);
    double maxZErrorInFile = *((const double*)ptr);

    if (height > 20000 || width > 20000)    // guard against bogus numbers; size limitation for old Lerc1
      return ErrCode::Failed;

    lercInfo.nCols = width;
    lercInfo.nRows = height;
    lercInfo.dt = Lerc::DT_Float;
    lercInfo.maxZError = maxZErrorInFile;

    Byte* pByte = const_cast<Byte*>(pLercBlob);
    bool onlyZPart = false;

    while (lercInfo.blobSize + numBytesHeader < numBytesBlob)    // means there could be another band
    {
      if (!cntZImg.read(&pByte, 1e12, false, onlyZPart))
        return (lercInfo.nBands > 0) ? ErrCode::Ok : ErrCode::Failed;    // no other band, we are done

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
      lercInfo.zMin = std::min(lercInfo.zMin, (double)zMin);
      lercInfo.zMax = std::max(lercInfo.zMax, (double)zMax);
    }

    return ErrCode::Ok;
  }

  return ErrCode::Failed;
}

// -------------------------------------------------------------------------- ;

ErrCode Lerc::Decode(const Byte* pLercBlob,
  unsigned int numBytesBlob,
  BitMask* pBitMask,                       // gets filled if not 0, even if all valid
  int nCols, int nRows, int nBands,
  DataType dt,
  void* pData)                             // outgoing data bands
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
    return ErrCode::WrongParam;
  }

  return ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;
// -------------------------------------------------------------------------- ;

template<class T>
ErrCode Lerc::ComputeCompressedSizeTempl(const T* pData,    // raw image data, row by row, band by band
  int nCols, int nRows, int nBands,
  const BitMask* pBitMask,                             // 0 if all pixels are valid
  double maxZErr,                                      // max coding error per pixel, or precision
  unsigned int& numBytesNeeded)                        // size of outgoing Lerc blob
{
  numBytesNeeded = 0;

  if (!pData || nCols <= 0 || nRows <= 0 || nBands <= 0 || maxZErr < 0)
    return ErrCode::WrongParam;

  if (pBitMask && (pBitMask->GetHeight() != nRows || pBitMask->GetWidth() != nCols))
    return ErrCode::WrongParam;

  Lerc2 lerc2;
  bool rv = pBitMask ? lerc2.Set(*pBitMask) : lerc2.Set(nCols, nRows);
  if (!rv)
    return ErrCode::Failed;

  // loop over the bands
  for (int iBand = 0; iBand < nBands; iBand++)
  {
    bool encMsk = (iBand == 0);    // store bit mask with first band only
    unsigned int nBytes = lerc2.ComputeNumBytesNeededToWrite(pData + nCols * nRows * iBand, maxZErr, encMsk);
    if (nBytes <= 0)
      return ErrCode::Failed;

    numBytesNeeded += nBytes;
  }

  return ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;

template<class T>
ErrCode Lerc::EncodeTempl(const T* pData,    // raw image data, row by row, band by band
  int nCols, int nRows, int nBands,
  const BitMask* pBitMask,                   // 0 if all pixels are valid
  double maxZErr,                            // max coding error per pixel, or precision
  Byte* pBuffer,                             // buffer to write to, function will fail if buffer too small
  unsigned int numBytesBuffer,               // buffer size
  unsigned int& numBytesWritten)             // num bytes written to buffer
{
  numBytesWritten = 0;

  if (!pData || nCols <= 0 || nRows <= 0 || nBands <= 0 || maxZErr < 0 || !pBuffer || !numBytesBuffer)
    return ErrCode::WrongParam;

  if (pBitMask && (pBitMask->GetHeight() != nRows || pBitMask->GetWidth() != nCols))
    return ErrCode::WrongParam;

  Lerc2 lerc2;
  bool rv = pBitMask ? lerc2.Set(*pBitMask) : lerc2.Set(nCols, nRows);
  if (!rv)
    return ErrCode::Failed;

  Byte* pByte = pBuffer;

  // loop over the bands, encode into array of single band Lerc blobs
  for (int iBand = 0; iBand < nBands; iBand++)
  {
    bool encMsk = (iBand == 0);    // store bit mask with first band only
    const T* arr = pData + nCols * nRows * iBand;

    unsigned int nBytes = lerc2.ComputeNumBytesNeededToWrite(arr, maxZErr, encMsk);
    if (nBytes == 0)
      return ErrCode::Failed;

    unsigned int nBytesAlloc = nBytes;

    if ((size_t)(pByte - pBuffer) + nBytesAlloc > numBytesBuffer)    // check we have enough space left
      return ErrCode::BufferTooSmall;

    if (!lerc2.Encode(arr, &pByte))
      return ErrCode::Failed;
  }

  numBytesWritten = (unsigned int)(pByte - pBuffer);
  return ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;

template<class T>
ErrCode Lerc::EncodeTempl(const T* pData,    // raw image data, row by row, band by band
  int nCols, int nRows, int nBands,
  const BitMask* pBitMask,                   // 0 if all pixels are valid
  double maxZErr,                            // max coding error per pixel, or precision
  Byte** ppLercBlob,                         // outgoing Lerc blob, delete[] when done
  unsigned int& numBytesBlob)                // size of outgoing Lerc blob
{
  numBytesBlob = 0;

  if (!pData || nCols <= 0 || nRows <= 0 || nBands <= 0 || maxZErr < 0 || !ppLercBlob)
    return ErrCode::WrongParam;

  if (pBitMask && (pBitMask->GetHeight() != nRows || pBitMask->GetWidth() != nCols))
    return ErrCode::WrongParam;

  size_t numBytesAllBands = 0;
  std::vector<size_t> numBytesPerBandVec(nBands, 0);
  std::vector<Byte*> bufferPerBandVec(nBands, 0);

  Lerc2 lerc2;
  bool rv = pBitMask ? lerc2.Set(*pBitMask) : lerc2.Set(nCols, nRows);
  if (!rv)
    return ErrCode::Failed;

  // loop over the bands, encode into array of single band Lerc blobs
  for (int iBand = 0; iBand < nBands; iBand++)
  {
    bool encMsk = (iBand == 0);    // store bit mask with first band only
    const T* arr = pData + nCols * nRows * iBand;

    unsigned int nBytes = lerc2.ComputeNumBytesNeededToWrite(arr, maxZErr, encMsk);
    if (nBytes == 0)
    {
      FreeBuffer(bufferPerBandVec);
      return ErrCode::Failed;
    }

    unsigned int nBytesAlloc = nBytes;
    bufferPerBandVec[iBand] = new Byte[nBytesAlloc];
    Byte* pByte = bufferPerBandVec[iBand];

    if (!lerc2.Encode(arr, &pByte))
    {
      FreeBuffer(bufferPerBandVec);
      return ErrCode::Failed;
    }

    numBytesPerBandVec[iBand] = pByte - bufferPerBandVec[iBand];    // num bytes really used
    numBytesAllBands += numBytesPerBandVec[iBand];
  }

  numBytesBlob = (unsigned int)numBytesAllBands;

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

  return ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;

template<class T>
ErrCode Lerc::DecodeTempl(T* pData,    // outgoing data bands
  const Byte* pLercBlob,
  unsigned int numBytesBlob,
  int nCols, int nRows, int nBands,
  BitMask* pBitMask)                   // gets filled if not 0, even if all valid
{
  if (!pData || nCols <= 0 || nRows <= 0 || nBands <= 0 || !pLercBlob || !numBytesBlob)
    return ErrCode::WrongParam;

  if (pBitMask && (pBitMask->GetHeight() != nRows || pBitMask->GetWidth() != nCols))
    return ErrCode::WrongParam;

  const Byte* pByte = pLercBlob;
  Byte* pByte1 = const_cast<Byte*>(pLercBlob);

  Lerc2 lerc2;
  CntZImage zImg;

  for (int iBand = 0; iBand < nBands; iBand++)
  {
    // first try Lerc2
    unsigned int minNumBytesHeader = Lerc2::MinNumBytesNeededToReadHeader();
    Lerc2::HeaderInfo hdInfo;
    if (((size_t)(pByte - pLercBlob) + minNumBytesHeader <= numBytesBlob) && Lerc2::GetHeaderInfo(pByte, hdInfo))
    {
      if (hdInfo.nCols != nCols || hdInfo.nRows != nRows)
        return ErrCode::Failed;

      if ((pByte - pLercBlob) + (size_t)hdInfo.blobSize > numBytesBlob)
        return ErrCode::BufferTooSmall;

      T* arr = pData + nCols * nRows * iBand;

      if (!lerc2.Decode(&pByte, arr, (pBitMask && iBand == 0) ? pBitMask->Bits() : 0))
        return ErrCode::Failed;
    }
    else    // then try Lerc1
    {
      unsigned int numBytesHeader = CntZImage::computeNumBytesNeededToReadHeader();

      if ((size_t)(pByte - pLercBlob) + numBytesHeader > numBytesBlob)
        return ErrCode::BufferTooSmall;

      bool onlyZPart = iBand > 0;
      if (!zImg.read(&pByte1, 1e12, false, onlyZPart))
        return ErrCode::Failed;

      if (zImg.getWidth() != nCols || zImg.getHeight() != nRows)
        return ErrCode::Failed;

      T* arr = pData + nCols * nRows * iBand;

      if (!Convert(zImg, arr, pBitMask))
        return ErrCode::Failed;
    }
  }  // iBand

  return ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;
// -------------------------------------------------------------------------- ;

void Lerc::FreeBuffer(std::vector<Byte*>& bufferVec)
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
bool Lerc::Convert(const CntZImage& zImg, T* arr, BitMask* pBitMask)
{
  if (!arr || !zImg.getSize())
    return false;

  const bool fltPnt = (typeid(*arr) == typeid(double)) || (typeid(*arr) == typeid(float));

  int h = zImg.getHeight();
  int w = zImg.getWidth();

  if (pBitMask && (pBitMask->GetHeight() != h || pBitMask->GetWidth() != w))
    return false;

  if (pBitMask)
    pBitMask->SetAllValid();

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

