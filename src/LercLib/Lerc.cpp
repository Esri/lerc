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

#include "Defines.h"
#include "Lerc.h"
#include "Lerc2.h"
#include <typeinfo>
#include <limits>

#ifdef HAVE_LERC1_DECODE
#include "Lerc1Decode/CntZImage.h"
#endif

using namespace std;
USING_NAMESPACE_LERC

// -------------------------------------------------------------------------- ;

ErrCode Lerc::ComputeCompressedSize(const void* pData, int version, DataType dt, int nDim, int nCols, int nRows, int nBands,
  int nMasks, const Byte* pValidBytes, double maxZErr, unsigned int& numBytesNeeded)
{
  switch (dt)
  {
  case DT_Char:    return ComputeCompressedSizeTempl((const signed char*)pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, numBytesNeeded);
  case DT_Byte:    return ComputeCompressedSizeTempl((const Byte*)pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, numBytesNeeded);
  case DT_Short:   return ComputeCompressedSizeTempl((const short*)pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, numBytesNeeded);
  case DT_UShort:  return ComputeCompressedSizeTempl((const unsigned short*)pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, numBytesNeeded);
  case DT_Int:     return ComputeCompressedSizeTempl((const int*)pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, numBytesNeeded);
  case DT_UInt:    return ComputeCompressedSizeTempl((const unsigned int*)pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, numBytesNeeded);
  case DT_Float:   return ComputeCompressedSizeTempl((const float*)pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, numBytesNeeded);
  case DT_Double:  return ComputeCompressedSizeTempl((const double*)pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, numBytesNeeded);

  default:
    return ErrCode::WrongParam;
  }
}

// -------------------------------------------------------------------------- ;

ErrCode Lerc::Encode(const void* pData, int version, DataType dt, int nDim, int nCols, int nRows, int nBands,
  int nMasks, const Byte* pValidBytes, double maxZErr, Byte* pBuffer, unsigned int numBytesBuffer, unsigned int& numBytesWritten)
{
  switch (dt)
  {
  case DT_Char:    return EncodeTempl((const signed char*)pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_Byte:    return EncodeTempl((const Byte*)pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_Short:   return EncodeTempl((const short*)pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_UShort:  return EncodeTempl((const unsigned short*)pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_Int:     return EncodeTempl((const int*)pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_UInt:    return EncodeTempl((const unsigned int*)pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_Float:   return EncodeTempl((const float*)pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, pBuffer, numBytesBuffer, numBytesWritten);
  case DT_Double:  return EncodeTempl((const double*)pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, pBuffer, numBytesBuffer, numBytesWritten);

  default:
    return ErrCode::WrongParam;
  }
}

// -------------------------------------------------------------------------- ;

ErrCode Lerc::GetLercInfo(const Byte* pLercBlob, unsigned int numBytesBlob, struct LercInfo& lercInfo)
{
  lercInfo.RawInit();

  // first try Lerc2
  struct Lerc2::HeaderInfo lerc2Info;
  bool bHasMask = false;
  int nMasks = 0;

  if (Lerc2::GetHeaderInfo(pLercBlob, numBytesBlob, lerc2Info, bHasMask))
  {
    lercInfo.version = lerc2Info.version;
    lercInfo.nDim = lerc2Info.nDim;
    lercInfo.nCols = lerc2Info.nCols;
    lercInfo.nRows = lerc2Info.nRows;
    lercInfo.numValidPixel = lerc2Info.numValidPixel;    // for 1st band
    lercInfo.nBands = 1;
    lercInfo.blobSize = lerc2Info.blobSize;
    lercInfo.dt = (DataType)lerc2Info.dt;
    lercInfo.zMin = lerc2Info.zMin;
    lercInfo.zMax = lerc2Info.zMax;
    lercInfo.maxZError = lerc2Info.maxZError;

    if (bHasMask || lercInfo.numValidPixel == 0)
      nMasks = 1;

    if (lercInfo.blobSize > (int)numBytesBlob)    // truncated blob, we won't be able to read this band
      return ErrCode::BufferTooSmall;

    struct Lerc2::HeaderInfo hdInfo;
    while (Lerc2::GetHeaderInfo(pLercBlob + lercInfo.blobSize, numBytesBlob - lercInfo.blobSize, hdInfo, bHasMask))
    {
      if (hdInfo.nDim != lercInfo.nDim
       || hdInfo.nCols != lercInfo.nCols
       || hdInfo.nRows != lercInfo.nRows
       || (int)hdInfo.dt != (int)lercInfo.dt)
       //|| hdInfo.maxZError != lercInfo.maxZError)    // with the new bitplane compression, maxZError can vary between bands
      {
        return ErrCode::Failed;
      }

      if (bHasMask || hdInfo.numValidPixel != lercInfo.numValidPixel)    // support mask per band
        nMasks = 2;

      if (lercInfo.blobSize > std::numeric_limits<int>::max() - hdInfo.blobSize)
        return ErrCode::Failed;

      lercInfo.blobSize += hdInfo.blobSize;

      if (lercInfo.blobSize > (int)numBytesBlob)    // truncated blob, we won't be able to read this band
        return ErrCode::BufferTooSmall;

      lercInfo.nBands++;
      lercInfo.zMin = min(lercInfo.zMin, hdInfo.zMin);
      lercInfo.zMax = max(lercInfo.zMax, hdInfo.zMax);
      lercInfo.maxZError = max(lercInfo.maxZError, hdInfo.maxZError);  // with the new bitplane compression, maxZError can vary between bands
    }

    lercInfo.nMasks = nMasks > 1 ? lercInfo.nBands : nMasks;

    return ErrCode::Ok;
  }


#ifdef HAVE_LERC1_DECODE
  // only if not Lerc2, try legacy Lerc1
  unsigned int numBytesHeaderBand0 = CntZImage::computeNumBytesNeededToReadHeader(false);
  unsigned int numBytesHeaderBand1 = CntZImage::computeNumBytesNeededToReadHeader(true);
  Byte* pByte = const_cast<Byte*>(pLercBlob);

  lercInfo.zMin =  FLT_MAX;
  lercInfo.zMax = -FLT_MAX;

  CntZImage cntZImg;
  if (numBytesHeaderBand0 <= numBytesBlob && cntZImg.read(&pByte, 1e12, true))    // read just the header
  {
    size_t nBytesRead = pByte - pLercBlob;
    size_t nBytesNeeded = 10 + 4 * sizeof(int) + 1 * sizeof(double);

    if (nBytesRead < nBytesNeeded)
      return ErrCode::Failed;

    Byte* ptr = const_cast<Byte*>(pLercBlob);
    ptr += 10 + 2 * sizeof(int);

    int height(0), width(0);
    memcpy(&height, ptr, sizeof(int));  ptr += sizeof(int);
    memcpy(&width,  ptr, sizeof(int));  ptr += sizeof(int);
    double maxZErrorInFile(0);
    memcpy(&maxZErrorInFile, ptr, sizeof(double));

    if (height > 20000 || width > 20000)    // guard against bogus numbers; size limitation for old Lerc1
      return ErrCode::Failed;

    lercInfo.nDim = 1;
    lercInfo.nCols = width;
    lercInfo.nRows = height;
    lercInfo.dt = Lerc::DT_Float;
    lercInfo.maxZError = maxZErrorInFile;

    Byte* pByte = const_cast<Byte*>(pLercBlob);
    bool onlyZPart = false;

    while (lercInfo.blobSize + numBytesHeaderBand1 < numBytesBlob)    // means there could be another band
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
      lercInfo.nMasks = numValidPixels < width * height ? 1 : 0;
    }

    return ErrCode::Ok;
  }
#endif

  return ErrCode::Failed;
}

// -------------------------------------------------------------------------- ;

ErrCode Lerc::Decode(const Byte* pLercBlob, unsigned int numBytesBlob, int nMasks, Byte* pValidBytes,
  int nDim, int nCols, int nRows, int nBands, DataType dt, void* pData)
{
  switch (dt)
  {
  case DT_Char:    return DecodeTempl((signed char*)pData, pLercBlob, numBytesBlob, nDim, nCols, nRows, nBands, nMasks, pValidBytes);
  case DT_Byte:    return DecodeTempl((Byte*)pData, pLercBlob, numBytesBlob, nDim, nCols, nRows, nBands, nMasks, pValidBytes);
  case DT_Short:   return DecodeTempl((short*)pData, pLercBlob, numBytesBlob, nDim, nCols, nRows, nBands, nMasks, pValidBytes);
  case DT_UShort:  return DecodeTempl((unsigned short*)pData, pLercBlob, numBytesBlob, nDim, nCols, nRows, nBands, nMasks, pValidBytes);
  case DT_Int:     return DecodeTempl((int*)pData, pLercBlob, numBytesBlob, nDim, nCols, nRows, nBands, nMasks, pValidBytes);
  case DT_UInt:    return DecodeTempl((unsigned int*)pData, pLercBlob, numBytesBlob, nDim, nCols, nRows, nBands, nMasks, pValidBytes);
  case DT_Float:   return DecodeTempl((float*)pData, pLercBlob, numBytesBlob, nDim, nCols, nRows, nBands, nMasks, pValidBytes);
  case DT_Double:  return DecodeTempl((double*)pData, pLercBlob, numBytesBlob, nDim, nCols, nRows, nBands, nMasks, pValidBytes);

  default:
    return ErrCode::WrongParam;
  }
}

// -------------------------------------------------------------------------- ;

ErrCode Lerc::ConvertToDouble(const void* pDataIn, DataType dt, size_t nDataValues, double* pDataOut)
{
  switch (dt)
  {
  case DT_Char:    return ConvertToDoubleTempl((const signed char*)pDataIn, nDataValues, pDataOut);
  case DT_Byte:    return ConvertToDoubleTempl((const Byte*)pDataIn, nDataValues, pDataOut);
  case DT_Short:   return ConvertToDoubleTempl((const short*)pDataIn, nDataValues, pDataOut);
  case DT_UShort:  return ConvertToDoubleTempl((const unsigned short*)pDataIn, nDataValues, pDataOut);
  case DT_Int:     return ConvertToDoubleTempl((const int*)pDataIn, nDataValues, pDataOut);
  case DT_UInt:    return ConvertToDoubleTempl((const unsigned int*)pDataIn, nDataValues, pDataOut);
  case DT_Float:   return ConvertToDoubleTempl((const float*)pDataIn, nDataValues, pDataOut);
  //case DT_Double:  no convert double to double

  default:
    return ErrCode::WrongParam;
  }
}

// -------------------------------------------------------------------------- ;
// -------------------------------------------------------------------------- ;

template<class T>
ErrCode Lerc::ComputeCompressedSizeTempl(const T* pData, int version, int nDim, int nCols, int nRows, int nBands,
  int nMasks, const Byte* pValidBytes, double maxZErr, unsigned int& numBytesNeeded)
{
  numBytesNeeded = 0;

  if (!pData || nDim <= 0 || nCols <= 0 || nRows <= 0 || nBands <= 0 || maxZErr < 0)
    return ErrCode::WrongParam;

  if (!(nMasks == 0 || nMasks == 1 || nMasks == nBands) || (nMasks > 0 && !pValidBytes))
    return ErrCode::WrongParam;

  unsigned int numBytesWritten = 0;

  return EncodeInternal(pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr,
    numBytesNeeded, nullptr, 0, numBytesWritten);
}

// -------------------------------------------------------------------------- ;

template<class T>
ErrCode Lerc::EncodeTempl(const T* pData, int version, int nDim, int nCols, int nRows, int nBands,
  int nMasks, const Byte* pValidBytes, double maxZErr, Byte* pBuffer, unsigned int numBytesBuffer, unsigned int& numBytesWritten)
{
  numBytesWritten = 0;

  if (!pData || nDim <= 0 || nCols <= 0 || nRows <= 0 || nBands <= 0 || maxZErr < 0 || !pBuffer || !numBytesBuffer)
    return ErrCode::WrongParam;

  if (!(nMasks == 0 || nMasks == 1 || nMasks == nBands) || (nMasks > 0 && !pValidBytes))
    return ErrCode::WrongParam;

  unsigned int numBytesNeeded = 0;

  return EncodeInternal(pData, version, nDim, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr,
    numBytesNeeded, pBuffer, numBytesBuffer, numBytesWritten);
}

// -------------------------------------------------------------------------- ;

template<class T>
ErrCode Lerc::DecodeTempl(T* pData, const Byte* pLercBlob, unsigned int numBytesBlob,
  int nDim, int nCols, int nRows, int nBands, int nMasks, Byte* pValidBytes)
{
  if (!pData || nDim <= 0 || nCols <= 0 || nRows <= 0 || nBands <= 0 || !pLercBlob || !numBytesBlob)
    return ErrCode::WrongParam;

  if (!(nMasks == 0 || nMasks == 1 || nMasks == nBands) || (nMasks > 0 && !pValidBytes))
    return ErrCode::WrongParam;

  const Byte* pByte = pLercBlob;
  Lerc2::HeaderInfo hdInfo;
  bool bHasMask = false;

  if (Lerc2::GetHeaderInfo(pByte, numBytesBlob, hdInfo, bHasMask) && hdInfo.version >= 1)    // if Lerc2
  {
    LercInfo lercInfo;
    ErrCode errCode = GetLercInfo(pLercBlob, numBytesBlob, lercInfo);    // fast for Lerc2
    if (errCode != ErrCode::Ok)
      return errCode;

    const int nMasksEncoded = lercInfo.nMasks;    // 0, 1, or nBands

    if (nMasks < nMasksEncoded)
      return ErrCode::WrongParam;

    size_t nBytesRemaining = numBytesBlob;
    Lerc2 lerc2;
    BitMask bitMask;

    for (int iBand = 0; iBand < nBands; iBand++)
    {
      if (((size_t)(pByte - pLercBlob) < numBytesBlob) && Lerc2::GetHeaderInfo(pByte, nBytesRemaining, hdInfo, bHasMask))
      {
        if (hdInfo.nDim != nDim || hdInfo.nCols != nCols || hdInfo.nRows != nRows)
          return ErrCode::Failed;

        if ((pByte - pLercBlob) + (size_t)hdInfo.blobSize > numBytesBlob)
          return ErrCode::BufferTooSmall;

        size_t nPix = (size_t)iBand * nRows * nCols;
        T* arr = pData + nPix * nDim;

        bool bGetMask = iBand < nMasks;

        if (bGetMask && !bitMask.SetSize(nCols, nRows))
          return ErrCode::Failed;

        if (!lerc2.Decode(&pByte, nBytesRemaining, arr, bGetMask ? bitMask.Bits() : nullptr))
          return ErrCode::Failed;

        if (bGetMask && !Convert(bitMask, pValidBytes + nPix))
          return ErrCode::Failed;
      }
    }  // iBand
  }  // Lerc2

  else    // might be old Lerc1
  {
#ifdef HAVE_LERC1_DECODE
    unsigned int numBytesHeaderBand0 = CntZImage::computeNumBytesNeededToReadHeader(false);
    unsigned int numBytesHeaderBand1 = CntZImage::computeNumBytesNeededToReadHeader(true);
    Byte* pByte1 = const_cast<Byte*>(pLercBlob);
    CntZImage zImg;

    for (int iBand = 0; iBand < nBands; iBand++)
    {
      unsigned int numBytesHeader = iBand == 0 ? numBytesHeaderBand0 : numBytesHeaderBand1;
      if ((size_t)(pByte - pLercBlob) + numBytesHeader > numBytesBlob)
        return ErrCode::BufferTooSmall;

      bool onlyZPart = iBand > 0;
      if (!zImg.read(&pByte1, 1e12, false, onlyZPart))
        return ErrCode::Failed;

      if (zImg.getWidth() != nCols || zImg.getHeight() != nRows)
        return ErrCode::Failed;

      size_t nPix = (size_t)iBand * nRows * nCols;
      T* arr = pData + nPix;
      Byte* pDst = iBand < nMasks ? pValidBytes + nPix : nullptr;

      if (!Convert(zImg, arr, pDst, iBand == 0))
        return ErrCode::Failed;
    }
#else
    return ErrCode::Failed;
#endif
  }

  return ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;
// -------------------------------------------------------------------------- ;

template<class T>
ErrCode Lerc::EncodeInternal(const T* pData, int version, int nDim, int nCols, int nRows, int nBands,
  int nMasks, const Byte* pValidBytes, double maxZErr, unsigned int& numBytesNeeded,
  Byte* pBuffer, unsigned int numBytesBuffer, unsigned int& numBytesWritten)
{
  numBytesNeeded = 0;
  numBytesWritten = 0;

  Lerc2 lerc2;
  if (version >= 0 && !lerc2.SetEncoderToOldVersion(version))
    return ErrCode::WrongParam;

  Byte* pDst = pBuffer;

  const size_t nPix = (size_t)nCols * nRows;
  const size_t nElem = nPix * nDim;

  const Byte* pPrevByteMask = nullptr;
  vector<T> dataBuffer;
  vector<Byte> maskBuffer, prevMaskBuffer;
  BitMask bitMask;

  // loop over the bands
  for (int iBand = 0; iBand < nBands; iBand++)
  {
    bool bEncMsk = (iBand == 0);

    // using the proper section of valid bytes, check this band for NaN
    const T* arr = pData + nElem * iBand;
    const Byte* pByteMask = (nMasks > 0) ? (pValidBytes + ((nMasks > 1) ? nPix * iBand : 0)) : nullptr;

    ErrCode errCode = CheckForNaN(arr, nDim, nCols, nRows, pByteMask);
    if (errCode != ErrCode::Ok && errCode != ErrCode::NaN)
      return errCode;

    if (errCode == ErrCode::NaN)    // found NaN values
    {
      if (!Resize(dataBuffer, nElem) || !Resize(maskBuffer, nPix))
        return ErrCode::Failed;

      memcpy(&dataBuffer[0], arr, nElem * sizeof(T));
      pByteMask ? memcpy(&maskBuffer[0], pByteMask, nPix) : memset(&maskBuffer[0], 1, nPix);

      if (!ReplaceNaNValues(dataBuffer, maskBuffer, nDim, nCols, nRows))
        return ErrCode::Failed;

      if (iBand > 0 && MasksDiffer(&maskBuffer[0], pPrevByteMask, nPix))
        bEncMsk = true;

      if (iBand < nBands - 1)
      {
        // keep current mask as new previous band mask
        prevMaskBuffer = maskBuffer;
        pPrevByteMask = &prevMaskBuffer[0];
      }

      arr = &dataBuffer[0];
      pByteMask = &maskBuffer[0];
    }

    else    // no NaN in this band, the common case
    {
      if (iBand > 0 && MasksDiffer(pByteMask, pPrevByteMask, nPix))
        bEncMsk = true;

      pPrevByteMask = pByteMask;
    }

    if (bEncMsk)
    {
      if (pByteMask && !Convert(pByteMask, nCols, nRows, bitMask))
        return ErrCode::Failed;

      if (!lerc2.Set(nDim, nCols, nRows, pByteMask ? bitMask.Bits() : nullptr))
        return ErrCode::Failed;
    }

    unsigned int nBytes = lerc2.ComputeNumBytesNeededToWrite(arr, maxZErr, bEncMsk);
    if (nBytes <= 0)
      return ErrCode::Failed;

    numBytesNeeded += nBytes;

    if (pBuffer)
    {
      if ((size_t)(pDst - pBuffer) + nBytes > numBytesBuffer)    // check we have enough space left
        return ErrCode::BufferTooSmall;

      if (!lerc2.Encode(arr, &pDst))
        return ErrCode::Failed;
    }
  }

  numBytesWritten = (unsigned int)(pDst - pBuffer);
  return ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;

#ifdef HAVE_LERC1_DECODE
template<class T>
bool Lerc::Convert(const CntZImage& zImg, T* arr, Byte* pByteMask, bool bMustFillMask)
{
  if (!arr || !zImg.getSize())
    return false;

  const bool fltPnt = (typeid(*arr) == typeid(double)) || (typeid(*arr) == typeid(float));

  int h = zImg.getHeight();
  int w = zImg.getWidth();

  const CntZ* srcPtr = zImg.getData();
  T* dstPtr = arr;
  int num = w * h;

  if (pByteMask)
  {
    memset(pByteMask, 0, num);

    for (int k = 0; k < num; k++)
    {
      if (srcPtr->cnt > 0)
      {
        *dstPtr = fltPnt ? (T)srcPtr->z : (T)floor(srcPtr->z + 0.5);
        pByteMask[k] = 1;
      }

      srcPtr++;
      dstPtr++;
    }
  }
  else
  {
    for (int k = 0; k < num; k++)
    {
      if (srcPtr->cnt > 0)
      {
        *dstPtr = fltPnt ? (T)srcPtr->z : (T)floor(srcPtr->z + 0.5);
      }
      else if (bMustFillMask)
        return false;

      srcPtr++;
      dstPtr++;
    }
  }

  return true;
}
#endif

// -------------------------------------------------------------------------- ;

template<class T>
ErrCode Lerc::ConvertToDoubleTempl(const T* pDataIn, size_t nDataValues, double* pDataOut)
{
  if (!pDataIn || !nDataValues || !pDataOut)
    return ErrCode::WrongParam;

  for (size_t k = 0; k < nDataValues; k++)
    pDataOut[k] = pDataIn[k];

  return ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;

template<class T> ErrCode Lerc::CheckForNaN(const T* arr, int nDim, int nCols, int nRows, const Byte* pByteMask)
{
  if (!arr || nDim <= 0 || nCols <= 0 || nRows <= 0)
    return ErrCode::WrongParam;

  if (typeid(T) != typeid(double) && typeid(T) != typeid(float))
    return ErrCode::Ok;

  for (size_t k = 0, i = 0; i < (size_t)nRows; i++)
  {
    bool bFoundNaN = false;
    const T* rowArr = &(arr[i * nCols * nDim]);

    if (!pByteMask)    // all valid
    {
      size_t num = (size_t)nCols * nDim;
      for (size_t m = 0; m < num; m++)
        if (std::isnan((double)rowArr[m]))
          bFoundNaN = true;
    }
    else    // not all valid
    {
      for (size_t n = 0, j = 0; j < (size_t)nCols; j++, k++, n += nDim)
        if (pByteMask[k])
        {
          for (int m = 0; m < nDim; m++)
            if (std::isnan((double)rowArr[n + m]))
              bFoundNaN = true;
        }
    }

    if (bFoundNaN)
      return ErrCode::NaN;
  }

  return ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;

template<class T> bool Lerc::ReplaceNaNValues(std::vector<T>& dataBuffer, std::vector<Byte>& maskBuffer, int nDim, int nCols, int nRows)
{
  if (nDim <= 0 || nCols <= 0 || nRows <= 0 || dataBuffer.size() != (size_t)nDim * nCols * nRows || maskBuffer.size() != (size_t)nCols * nRows)
    return false;

  bool bIsFloat = (typeid(T) == typeid(float));
  const T noDataValue = (T)(bIsFloat ? -FLT_MAX : -DBL_MAX);

  for (size_t k = 0, i = 0; i < (size_t)nRows; i++)
  {
    T* rowArr = &(dataBuffer[i * nCols * nDim]);

    for (size_t n = 0, j = 0; j < (size_t)nCols; j++, k++, n += nDim)
    {
      if (maskBuffer[k])
      {
        int cntNaN = 0;

        for (int m = 0; m < nDim; m++)
          if (std::isnan((double)rowArr[n + m]))
          {
            cntNaN++;
            rowArr[n + m] = noDataValue;
          }

        if (cntNaN == nDim)
          maskBuffer[k] = 0;
      }
    }
  }

  return true;
}

// -------------------------------------------------------------------------- ;

template<class T> bool Lerc::Resize(std::vector<T>& buffer, size_t nElem)
{
  try
  {
    buffer.resize(nElem);
  }
  catch (...)
  {
    return false;
  }

  return true;
}

// -------------------------------------------------------------------------- ;

bool Lerc::Convert(const Byte* pByteMask, int nCols, int nRows, BitMask& bitMask)
{
  if (!pByteMask || nCols <= 0 || nRows <= 0)
    return false;

  if (!bitMask.SetSize(nCols, nRows))
    return false;

  bitMask.SetAllValid();

  for (int k = 0, i = 0; i < nRows; i++)
    for (int j = 0; j < nCols; j++, k++)
      if (!pByteMask[k])
        bitMask.SetInvalid(k);

  return true;
}

// -------------------------------------------------------------------------- ;

bool Lerc::Convert(const BitMask& bitMask, Byte* pByteMask)
{
  int nCols = bitMask.GetWidth();
  int nRows = bitMask.GetHeight();

  if (nCols <= 0 || nRows <= 0 || !pByteMask)
    return false;

  memset(pByteMask, 0, (size_t)nCols * nRows);

  for (int k = 0, i = 0; i < nRows; i++)
    for (int j = 0; j < nCols; j++, k++)
      if (bitMask.IsValid(k))
        pByteMask[k] = 1;

  return true;
}

// -------------------------------------------------------------------------- ;

bool Lerc::MasksDiffer(const Byte* p0, const Byte* p1, size_t n)
{
  if (p0 == p1)
    return false;

  if (!p0)    // means all valid
    return memchr(p1, 0, n);    // any invalid?
  else if (!p1)
    return memchr(p0, 0, n);
  else
    return memcmp(p0, p1, n);
}

// -------------------------------------------------------------------------- ;

