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

#include "Defines.h"
#include "Lerc.h"
#include "Lerc2.h"
#include <typeinfo>
#include <limits>
#include <functional>

#ifdef HAVE_LERC1_DECODE
  #include "Lerc1Decode/CntZImage.h"
#endif

using namespace std;
USING_NAMESPACE_LERC

// -------------------------------------------------------------------------- ;

ErrCode Lerc::ComputeCompressedSize(const void* pData, int version, DataType dt, int nDepth, int nCols, int nRows, int nBands,
  int nMasks, const Byte* pValidBytes, double maxZErr, unsigned int& numBytesNeeded, const unsigned char* pUsesNoData, const double* noDataValues)
{
#define LERC_ARG_1 version, nDepth, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, numBytesNeeded, pUsesNoData, noDataValues

  switch (dt)
  {
  case DT_Char:    return ComputeCompressedSizeTempl((const signed char*)pData, LERC_ARG_1);
  case DT_Byte:    return ComputeCompressedSizeTempl((const Byte*)pData, LERC_ARG_1);
  case DT_Short:   return ComputeCompressedSizeTempl((const short*)pData, LERC_ARG_1);
  case DT_UShort:  return ComputeCompressedSizeTempl((const unsigned short*)pData, LERC_ARG_1);
  case DT_Int:     return ComputeCompressedSizeTempl((const int*)pData, LERC_ARG_1);
  case DT_UInt:    return ComputeCompressedSizeTempl((const unsigned int*)pData, LERC_ARG_1);
  case DT_Float:   return ComputeCompressedSizeTempl((const float*)pData, LERC_ARG_1);
  case DT_Double:  return ComputeCompressedSizeTempl((const double*)pData, LERC_ARG_1);

  default:
    return ErrCode::WrongParam;
  }

#undef LERC_ARG_1
}

// -------------------------------------------------------------------------- ;

ErrCode Lerc::Encode(const void* pData, int version, DataType dt, int nDepth, int nCols, int nRows, int nBands,
  int nMasks, const Byte* pValidBytes, double maxZErr, Byte* pBuffer, unsigned int numBytesBuffer,
  unsigned int& numBytesWritten, const unsigned char* pUsesNoData, const double* noDataValues)
{
#define LERC_ARG_2 version, nDepth, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr, pBuffer, numBytesBuffer, numBytesWritten, pUsesNoData, noDataValues

  switch (dt)
  {
  case DT_Char:    return EncodeTempl((const signed char*)pData, LERC_ARG_2);
  case DT_Byte:    return EncodeTempl((const Byte*)pData, LERC_ARG_2);
  case DT_Short:   return EncodeTempl((const short*)pData, LERC_ARG_2);
  case DT_UShort:  return EncodeTempl((const unsigned short*)pData, LERC_ARG_2);
  case DT_Int:     return EncodeTempl((const int*)pData, LERC_ARG_2);
  case DT_UInt:    return EncodeTempl((const unsigned int*)pData, LERC_ARG_2);
  case DT_Float:   return EncodeTempl((const float*)pData, LERC_ARG_2);
  case DT_Double:  return EncodeTempl((const double*)pData, LERC_ARG_2);

  default:
    return ErrCode::WrongParam;
  }

#undef LERC_ARG_2
}

// -------------------------------------------------------------------------- ;

ErrCode Lerc::GetLercInfo(const Byte* pLercBlob, unsigned int numBytesBlob, struct LercInfo& lercInfo, double* pMins, double* pMaxs, size_t nElem)
{
  lercInfo.RawInit();

  // first try Lerc2
  struct Lerc2::HeaderInfo lerc2Info;
  bool bHasMask = false;
  int nMasks = 0;

  if (Lerc2::GetHeaderInfo(pLercBlob, numBytesBlob, lerc2Info, bHasMask))
  {
    lercInfo.version = lerc2Info.version;
    lercInfo.nDepth = lerc2Info.nDepth;
    lercInfo.nCols = lerc2Info.nCols;
    lercInfo.nRows = lerc2Info.nRows;
    lercInfo.numValidPixel = lerc2Info.numValidPixel;    // for 1st band
    lercInfo.blobSize = lerc2Info.blobSize;
    lercInfo.dt = (DataType)lerc2Info.dt;
    lercInfo.zMin = lerc2Info.zMin;
    lercInfo.zMax = lerc2Info.zMax;
    lercInfo.maxZError = lerc2Info.maxZError;
    lercInfo.nUsesNoDataValue = lerc2Info.bPassNoDataValues ? 1 : 0;

    bool bTryNextBlob = (lerc2Info.version <= 5) || (lerc2Info.nBlobsMore > 0);

    if (bHasMask || lercInfo.numValidPixel == 0)
      nMasks = 1;

    if (pMins && pMaxs)
    {
      ErrCode errCode = GetRanges(pLercBlob, numBytesBlob, 0, lerc2Info, pMins, pMaxs, nElem);    // band 0
      if (errCode != ErrCode::Ok)
        return errCode;
    }

    lercInfo.nBands = 1;

    if (lercInfo.blobSize > (int)numBytesBlob)    // truncated blob, we won't be able to read this band
      return ErrCode::BufferTooSmall;

    struct Lerc2::HeaderInfo hdInfo;
    while (bTryNextBlob && Lerc2::GetHeaderInfo(pLercBlob + lercInfo.blobSize, numBytesBlob - lercInfo.blobSize, hdInfo, bHasMask))
    {
      if (hdInfo.nDepth != lercInfo.nDepth
       || hdInfo.nCols != lercInfo.nCols
       || hdInfo.nRows != lercInfo.nRows
       || (int)hdInfo.dt != (int)lercInfo.dt)
      {
        return ErrCode::Failed;
      }

      bTryNextBlob = (hdInfo.version <= 5) || (hdInfo.nBlobsMore > 0);

      if (hdInfo.bPassNoDataValues)
        lercInfo.nUsesNoDataValue++;

      if (bHasMask || hdInfo.numValidPixel != lercInfo.numValidPixel)    // support mask per band
        nMasks = 2;

      if (lercInfo.blobSize > std::numeric_limits<int>::max() - hdInfo.blobSize)    // guard against overflow
        return ErrCode::Failed;

      if (lercInfo.blobSize + hdInfo.blobSize > (int)numBytesBlob)    // truncated blob, we won't be able to read this band
        return ErrCode::BufferTooSmall;

      lercInfo.zMin = min(lercInfo.zMin, hdInfo.zMin);
      lercInfo.zMax = max(lercInfo.zMax, hdInfo.zMax);
      lercInfo.maxZError = max(lercInfo.maxZError, hdInfo.maxZError);  // with the new bitplane compression, maxZError can vary between bands

      if (pMins && pMaxs)
      {
        ErrCode errCode = GetRanges(pLercBlob + lercInfo.blobSize, numBytesBlob - lercInfo.blobSize, lercInfo.nBands, hdInfo, pMins, pMaxs, nElem);
        if (errCode != ErrCode::Ok)
          return errCode;
      }

      lercInfo.blobSize += hdInfo.blobSize;
      lercInfo.nBands++;
    }

    lercInfo.nMasks = nMasks > 1 ? lercInfo.nBands : nMasks;
    
    if (lercInfo.nUsesNoDataValue > 0)
      lercInfo.nUsesNoDataValue = lercInfo.nBands;    // if there is any noData used in any band, allow for different noData per band

    return ErrCode::Ok;
  }


#ifdef HAVE_LERC1_DECODE
  // only if not Lerc2, try legacy Lerc1
  unsigned int numBytesHeaderBand0 = CntZImage::computeNumBytesNeededToReadHeader(false);
  unsigned int numBytesHeaderBand1 = CntZImage::computeNumBytesNeededToReadHeader(true);
  const Byte* pByte = pLercBlob;

  lercInfo.zMin =  FLT_MAX;
  lercInfo.zMax = -FLT_MAX;

  CntZImage cntZImg;
  if (numBytesHeaderBand0 <= numBytesBlob && cntZImg.read(&pByte, 1e12, true))    // read just the header
  {
    size_t nBytesRead = pByte - pLercBlob;
    size_t nBytesNeeded = 10 + 4 * sizeof(int) + 1 * sizeof(double);

    if (nBytesRead < nBytesNeeded)
      return ErrCode::Failed;

    const Byte* ptr = pLercBlob;
    ptr += 10 + 2 * sizeof(int);

    int height(0), width(0);
    memcpy(&height, ptr, sizeof(int));  ptr += sizeof(int);
    memcpy(&width,  ptr, sizeof(int));  ptr += sizeof(int);
    double maxZErrorInFile(0);
    memcpy(&maxZErrorInFile, ptr, sizeof(double));

    if (height > 20000 || width > 20000)    // guard against bogus numbers; size limitation for old Lerc1
      return ErrCode::Failed;

    lercInfo.nDepth = 1;
    lercInfo.nCols = width;
    lercInfo.nRows = height;
    lercInfo.dt = Lerc::DT_Float;
    lercInfo.maxZError = maxZErrorInFile;

    pByte = pLercBlob;
    bool onlyZPart = false;

    while (lercInfo.blobSize + numBytesHeaderBand1 < numBytesBlob)    // means there could be another band
    {
      if (!cntZImg.read(&pByte, 1e12, false, onlyZPart))
        return (lercInfo.nBands > 0) ? ErrCode::Ok : ErrCode::Failed;    // no other band, we are done

      onlyZPart = true;
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

      if (pMins && pMaxs)
      {
        pMins[lercInfo.nBands] = zMin;
        pMaxs[lercInfo.nBands] = zMax;
      }

      lercInfo.nBands++;
    }

    return ErrCode::Ok;
  }
#endif

  return ErrCode::Failed;
}

// -------------------------------------------------------------------------- ;

ErrCode Lerc::Decode(const Byte* pLercBlob, unsigned int numBytesBlob, int nMasks, Byte* pValidBytes,
  int nDepth, int nCols, int nRows, int nBands, DataType dt, void* pData, unsigned char* pUsesNoData, double* noDataValues)
{
#define LERC_ARG_3 pLercBlob, numBytesBlob, nDepth, nCols, nRows, nBands, nMasks, pValidBytes, pUsesNoData, noDataValues

  switch (dt)
  {
  case DT_Char:    return DecodeTempl((signed char*)pData, LERC_ARG_3);
  case DT_Byte:    return DecodeTempl((Byte*)pData, LERC_ARG_3);
  case DT_Short:   return DecodeTempl((short*)pData, LERC_ARG_3);
  case DT_UShort:  return DecodeTempl((unsigned short*)pData, LERC_ARG_3);
  case DT_Int:     return DecodeTempl((int*)pData, LERC_ARG_3);
  case DT_UInt:    return DecodeTempl((unsigned int*)pData, LERC_ARG_3);
  case DT_Float:   return DecodeTempl((float*)pData, LERC_ARG_3);
  case DT_Double:  return DecodeTempl((double*)pData, LERC_ARG_3);

  default:
    return ErrCode::WrongParam;
  }

#undef LERC_ARG_3
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
ErrCode Lerc::ComputeCompressedSizeTempl(const T* pData, int version, int nDepth, int nCols, int nRows,
  int nBands, int nMasks, const Byte* pValidBytes, double maxZErr, unsigned int& numBytesNeeded,
  const unsigned char* pUsesNoData, const double* noDataValues)
{
  numBytesNeeded = 0;

  if (!pData || nDepth <= 0 || nCols <= 0 || nRows <= 0 || nBands <= 0 || maxZErr < 0)
    return ErrCode::WrongParam;

  if (!(nMasks == 0 || nMasks == 1 || nMasks == nBands) || (nMasks > 0 && !pValidBytes))
    return ErrCode::WrongParam;

  unsigned int numBytesWritten = 0;

  if (version >= 0 && version <= 5)
  {
    if (pUsesNoData)
      for (int i = 0; i < nBands; i++)
        if (pUsesNoData[i])
          return ErrCode::WrongParam;

    return EncodeInternal_v5(pData, version, nDepth, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr,
      numBytesNeeded, nullptr, 0, numBytesWritten);
  }
  else
  {
    return EncodeInternal(pData, version, nDepth, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr,
      numBytesNeeded, nullptr, 0, numBytesWritten, pUsesNoData, noDataValues);
  }
}

// -------------------------------------------------------------------------- ;

template<class T>
ErrCode Lerc::EncodeTempl(const T* pData, int version, int nDepth, int nCols, int nRows, int nBands,
  int nMasks, const Byte* pValidBytes, double maxZErr, Byte* pBuffer, unsigned int numBytesBuffer,
  unsigned int& numBytesWritten, const unsigned char* pUsesNoData, const double* noDataValues)
{
  numBytesWritten = 0;

  if (!pData || nDepth <= 0 || nCols <= 0 || nRows <= 0 || nBands <= 0 || maxZErr < 0 || !pBuffer || !numBytesBuffer)
    return ErrCode::WrongParam;

  if (!(nMasks == 0 || nMasks == 1 || nMasks == nBands) || (nMasks > 0 && !pValidBytes))
    return ErrCode::WrongParam;

  unsigned int numBytesNeeded = 0;

  if (version >= 0 && version <= 5)
  {
    if (pUsesNoData)
      for (int i = 0; i < nBands; i++)
        if (pUsesNoData[i])
          return ErrCode::WrongParam;

    return EncodeInternal_v5(pData, version, nDepth, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr,
      numBytesNeeded, pBuffer, numBytesBuffer, numBytesWritten);
  }
  else
  {
    return EncodeInternal(pData, version, nDepth, nCols, nRows, nBands, nMasks, pValidBytes, maxZErr,
      numBytesNeeded, pBuffer, numBytesBuffer, numBytesWritten, pUsesNoData, noDataValues);
  }
}

// -------------------------------------------------------------------------- ;

template<class T>
ErrCode Lerc::DecodeTempl(T* pData, const Byte* pLercBlob, unsigned int numBytesBlob,
  int nDepth, int nCols, int nRows, int nBands, int nMasks, Byte* pValidBytes,
  unsigned char* pUsesNoData, double* noDataValues)
{
  if (!pData || nDepth <= 0 || nCols <= 0 || nRows <= 0 || nBands <= 0 || !pLercBlob || !numBytesBlob)
    return ErrCode::WrongParam;

  if (!(nMasks == 0 || nMasks == 1 || nMasks == nBands) || (nMasks > 0 && !pValidBytes))
    return ErrCode::WrongParam;

  const Byte* pByte = pLercBlob;
  Lerc2::HeaderInfo hdInfo;
  bool bHasMask = false;

  if (Lerc2::GetHeaderInfo(pByte, numBytesBlob, hdInfo, bHasMask) && hdInfo.version >= 1)    // if Lerc2
  {
    LercInfo lercInfo;
    ErrCode errCode = GetLercInfo(pLercBlob, numBytesBlob, lercInfo);    // fast for Lerc2, does most checks
    if (errCode != ErrCode::Ok)
      return errCode;

    // caller must provide enough space for the masks that are there (e.g., if they differ between bands)
    if (nMasks < lercInfo.nMasks)    // 0, 1, or nBands
      return ErrCode::WrongParam;

    // caller cannot ask for more bands than are there
    if (nBands > lercInfo.nBands)
      return ErrCode::WrongParam;

    // if Lerc blob has noData values not covered by the mask, caller must get it (make sure caller cannot miss it)
    if (lercInfo.nUsesNoDataValue && nDepth > 1)
    {
      if (!pUsesNoData || !noDataValues)
        return ErrCode::HasNoData;

      try
      {
        memset(pUsesNoData, 0, nBands);
        memset(noDataValues, 0, nBands * sizeof(double));
      }
      catch (...)
      {
        return ErrCode::HasNoData;
      }
    }

    size_t nBytesRemaining = numBytesBlob;
    Lerc2 lerc2;
    BitMask bitMask;

    for (int iBand = 0; iBand < nBands; iBand++)
    {
      if (((size_t)(pByte - pLercBlob) < numBytesBlob) && Lerc2::GetHeaderInfo(pByte, nBytesRemaining, hdInfo, bHasMask))
      {
        if (hdInfo.nDepth != nDepth || hdInfo.nCols != nCols || hdInfo.nRows != nRows)
          return ErrCode::Failed;

        if ((pByte - pLercBlob) + (size_t)hdInfo.blobSize > numBytesBlob)
          return ErrCode::BufferTooSmall;

        size_t nPix = (size_t)iBand * nRows * nCols;
        T* arr = pData + nPix * nDepth;

        bool bGetMask = iBand < nMasks;

        if (bGetMask && !bitMask.SetSize(nCols, nRows))
          return ErrCode::Failed;

        if (!lerc2.Decode(&pByte, nBytesRemaining, arr, bGetMask ? bitMask.Bits() : nullptr))
          return ErrCode::Failed;

        if (lercInfo.nUsesNoDataValue && nDepth > 1)
        {
          pUsesNoData[iBand] = hdInfo.bPassNoDataValues ? 1 : 0;
          noDataValues[iBand] = hdInfo.noDataValOrig;

          if (hdInfo.bPassNoDataValues && !RemapNoData(arr, bitMask, hdInfo))
            return ErrCode::Failed;
        }

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
    const Byte* pByte1 = pLercBlob;
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
ErrCode Lerc::EncodeInternal_v5(const T* pData, int version, int nDepth, int nCols, int nRows, int nBands,
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
  const size_t nElem = nPix * nDepth;

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

    ErrCode errCode = CheckForNaN(arr, nDepth, nCols, nRows, pByteMask);
    if (errCode != ErrCode::Ok && errCode != ErrCode::NaN)
      return errCode;

    if (errCode == ErrCode::NaN)    // found NaN values
    {
      if (!Resize(dataBuffer, nElem) || !Resize(maskBuffer, nPix))
        return ErrCode::Failed;

      memcpy(&dataBuffer[0], arr, nElem * sizeof(T));
      pByteMask ? memcpy(&maskBuffer[0], pByteMask, nPix) : memset(&maskBuffer[0], 1, nPix);

      if (!ReplaceNaNValues(dataBuffer, maskBuffer, nDepth, nCols, nRows))
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

      if (!lerc2.Set(nDepth, nCols, nRows, pByteMask ? bitMask.Bits() : nullptr))
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
  }  // iBand

  numBytesWritten = (unsigned int)(pDst - pBuffer);
  return ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;

template<class T>
ErrCode Lerc::EncodeInternal(const T* pData, int version, int nDepth, int nCols, int nRows, int nBands,
  int nMasks, const Byte* pValidBytes, double maxZErr, unsigned int& numBytesNeeded,
  Byte* pBuffer, unsigned int numBytesBuffer, unsigned int& numBytesWritten,
  const unsigned char* pUsesNoData, const double* noDataValues)
{
  numBytesNeeded = 0;
  numBytesWritten = 0;

  if (version >= 0 && version <= 5)
    return ErrCode::WrongParam;

  Lerc2 lerc2;

#ifdef ENCODE_VERIFY
  Lerc2 lerc2Verify;
#endif

  if (version >= 0 && !lerc2.SetEncoderToOldVersion(version))
    return ErrCode::WrongParam;

  if (pUsesNoData && !noDataValues)
    for (int i = 0; i < nBands; i++)
      if (pUsesNoData[i])
        return ErrCode::WrongParam;

  Byte* pDst = pBuffer;

  const size_t nPix = (size_t)nCols * nRows;
  const size_t nElem = nPix * nDepth;

  const Byte* pPrevByteMask = nullptr;
  vector<T> dataBuffer;
  vector<Byte> maskBuffer, prevMaskBuffer;
  BitMask bitMask;

  // allocate buffer for 1 band
  if (!Resize(dataBuffer, nElem) || !Resize(maskBuffer, nPix))
    return ErrCode::Failed;

  bool bIsFltOrDbl = (typeid(T) == typeid(float) || typeid(T) == typeid(double));
  bool bAnyMaskModified = false;
  ErrCode errCode = ErrCode::Ok;

  // loop over the bands
  for (int iBand = 0; iBand < nBands; iBand++)
  {
    bool bEncMsk = (iBand == 0);

    // get the data and mask for this band
    const T* arrOrig = pData + nElem * iBand;
    const Byte* pByteMaskOrig = (nMasks > 0) ? (pValidBytes + ((nMasks > 1) ? nPix * iBand : 0)) : nullptr;

    // copy to buffer so we can modify it
    memcpy(&dataBuffer[0], arrOrig, nElem * sizeof(T));
    pByteMaskOrig ? memcpy(&maskBuffer[0], pByteMaskOrig, nPix) : memset(&maskBuffer[0], 1, nPix);

    double maxZErrL = maxZErr;    // maxZErrL can get modified in the filter functions below

    bool bPassNoDataValue = (pUsesNoData && (pUsesNoData[iBand] > 0));
    const double noDataOrig = bPassNoDataValue ? noDataValues[iBand] : 0;

    double noDataL = noDataOrig;    // noDataL can get modified in the filter functions below

    bool bIsFltDblAllInt = false;    // are the flt numbers all integer really? Double can be used for int numbers beyond 32 bit range
    bool bModifiedMask = false;    // can turn true if NaN or noData found at valid pixels
    bool bNeedNoData = false;    // can only turn true for nDepth > 1, and mix of valid and invalid values at the same pixel (special case)
    errCode = ErrCode::Ok;

    if (bIsFltOrDbl)    // if flt type, filter out NaN and / or noData values and update the mask if possible
    {
      errCode = FilterNoDataAndNaN(dataBuffer, maskBuffer, nDepth, nCols, nRows, maxZErrL, bPassNoDataValue, noDataL,
        bModifiedMask, bNeedNoData, bIsFltDblAllInt);
    }
    else if (bPassNoDataValue)    // if int type (no NaN), and no noData value specified, nothing to do
    {
      errCode = FilterNoData(dataBuffer, maskBuffer, nDepth, nCols, nRows, maxZErrL, bPassNoDataValue, noDataL, bModifiedMask, bNeedNoData);
    }

    if (errCode != ErrCode::Ok)
      return errCode;

    if (bModifiedMask)
      bAnyMaskModified = true;

    bool bCompareMasks = (nMasks > 1) || bAnyMaskModified;

    if (bCompareMasks && (iBand > 0) && MasksDiffer(&maskBuffer[0], pPrevByteMask, nPix))
      bEncMsk = true;

    if (nBands > 1 && iBand < nBands - 1)
    {
      // keep current mask as new previous band mask
      prevMaskBuffer = maskBuffer;
      pPrevByteMask = &prevMaskBuffer[0];
    }

    const T* arrL = &dataBuffer[0];
    const Byte* pByteMaskL = &maskBuffer[0];

    if (bEncMsk)
    {
      bool bAllValid = !memchr(pByteMaskL, 0, nPix);

      if (!bAllValid && !Convert(pByteMaskL, nCols, nRows, bitMask))
        return ErrCode::Failed;

      if (!lerc2.Set(nDepth, nCols, nRows, !bAllValid ? bitMask.Bits() : nullptr))
        return ErrCode::Failed;
    }

    // set other flags

    if (!lerc2.SetNoDataValues(bNeedNoData, noDataL, noDataOrig))
      return ErrCode::Failed;

    if (!lerc2.SetNumBlobsMoreToCome(nBands - 1 - iBand))
      return ErrCode::Failed;

    if (!lerc2.SetIsAllInt(bIsFltDblAllInt))
      return ErrCode::Failed;

    unsigned int nBytes = lerc2.ComputeNumBytesNeededToWrite(arrL, maxZErrL, bEncMsk);
    if (nBytes <= 0)
      return ErrCode::Failed;

    numBytesNeeded += nBytes;

    if (pBuffer)
    {
      if ((size_t)(pDst - pBuffer) + nBytes > numBytesBuffer)    // check we have enough space left
        return ErrCode::BufferTooSmall;

#ifdef ENCODE_VERIFY
      const Byte* pDst0 = pDst;
#endif

      if (!lerc2.Encode(arrL, &pDst))
        return ErrCode::Failed;

#ifdef ENCODE_VERIFY
      size_t blobSize = pDst - pDst0;

      if (!DecodeAndCompareToInput(pDst0, blobSize, maxZErrL, lerc2Verify, arrL, pByteMaskL,
        arrOrig, pByteMaskOrig, bPassNoDataValue, noDataOrig, bModifiedMask))
      {
        return ErrCode::Failed;
      }
#endif

    }
  }  // iBand

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

template<class T> ErrCode Lerc::CheckForNaN(const T* arr, int nDepth, int nCols, int nRows, const Byte* pByteMask)
{
  if (!arr || nDepth <= 0 || nCols <= 0 || nRows <= 0)
    return ErrCode::WrongParam;

  if (typeid(T) != typeid(double) && typeid(T) != typeid(float))
    return ErrCode::Ok;

  for (size_t k = 0, i = 0; i < (size_t)nRows; i++)
  {
    bool bFoundNaN = false;
    const T* rowArr = &(arr[i * nCols * nDepth]);

    if (!pByteMask)    // all valid
    {
      size_t num = (size_t)nCols * nDepth;
      for (size_t m = 0; m < num; m++)
        if (std::isnan((double)rowArr[m]))
          bFoundNaN = true;
    }
    else    // not all valid
    {
      for (size_t n = 0, j = 0; j < (size_t)nCols; j++, k++, n += nDepth)
        if (pByteMask[k])
        {
          for (int m = 0; m < nDepth; m++)
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

template<class T> bool Lerc::ReplaceNaNValues(std::vector<T>& dataBuffer, std::vector<Byte>& maskBuffer, int nDepth, int nCols, int nRows)
{
  if (nDepth <= 0 || nCols <= 0 || nRows <= 0 || dataBuffer.size() != (size_t)nDepth * nCols * nRows || maskBuffer.size() != (size_t)nCols * nRows)
    return false;

  T noDataValue = 0;
  {
    #if defined _WIN32
      #pragma warning(disable: 4756)    // would trigger build warning in linux
    #endif

    noDataValue = (T)((typeid(T) == typeid(float)) ? -FLT_MAX : -DBL_MAX);
  }

  for (size_t k = 0, i = 0; i < (size_t)nRows; i++)
  {
    T* rowArr = &(dataBuffer[i * nCols * nDepth]);

    for (size_t n = 0, j = 0; j < (size_t)nCols; j++, k++, n += nDepth)
    {
      if (maskBuffer[k])
      {
        int cntNaN = 0;

        for (int m = 0; m < nDepth; m++)
          if (std::isnan((double)rowArr[n + m]))
          {
            cntNaN++;
            rowArr[n + m] = noDataValue;
          }

        if (cntNaN == nDepth)
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

ErrCode Lerc::GetRanges(const Byte* pLercBlob, unsigned int numBytesBlob, int iBand,
  const struct Lerc2::HeaderInfo& lerc2Info, double* pMins, double* pMaxs, size_t nElem)
{
  const int nDepth = lerc2Info.nDepth;

  if (nDepth <= 0 || iBand < 0 || !pMins || !pMaxs)
    return ErrCode::WrongParam;

  if (nElem < ((size_t)iBand + 1) * (size_t)nDepth)
    return ErrCode::BufferTooSmall;

  if (nDepth == 1)
  {
    pMins[iBand] = lerc2Info.zMin;
    pMaxs[iBand] = lerc2Info.zMax;
  }
  else
  {
    if (lerc2Info.bPassNoDataValues)    // for nDepth > 1, and mix of valid and invalid values at same pixel, better fail
      return ErrCode::HasNoData;        // than return min or max values that contain noData (to be fixed in next codec version)

    // read header, mask, ranges, and copy them out
    Lerc2 lerc2;
    if (!lerc2.GetRanges(pLercBlob, numBytesBlob, &pMins[iBand * nDepth], &pMaxs[iBand * nDepth]))
      return ErrCode::Failed;
  }

  return ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;

template<class T>
bool Lerc::RemapNoData(T* data, const BitMask& bitMask, const struct Lerc2::HeaderInfo& lerc2Info)
{
  int nCols = lerc2Info.nCols;
  int nRows = lerc2Info.nRows;
  int nDepth = lerc2Info.nDepth;

  if (!data || nCols <= 0 || nRows <= 0 || nDepth <= 0)
    return false;

  const T noDataOld = (T)lerc2Info.noDataVal;
  const T noDataNew = (T)lerc2Info.noDataValOrig;

  if (noDataNew != noDataOld)
  {
    bool bUseMask = (bitMask.GetWidth() == nCols) && (bitMask.GetHeight() == nRows);

    for (long k = 0, i = 0; i < nRows; i++)
    {
      T* rowArr = &(data[i * nCols * nDepth]);

      for (long n = 0, j = 0; j < nCols; j++, k++, n += nDepth)
        if (!bUseMask || bitMask.IsValid(k))
          for (long m = 0; m < nDepth; m++)
            if (rowArr[n + m] == noDataOld)
              rowArr[n + m] = noDataNew;
    }
  }

  return true;
}

// -------------------------------------------------------------------------- ;

template<class T>
bool Lerc::DecodeAndCompareToInput(const Byte* pLercBlob, size_t blobSize, double maxZErr, Lerc2& lerc2Verify,
  const T* pData, const Byte* pByteMask, const T* pDataOrig, const Byte* pByteMaskOrig,
  bool bInputHasNoData, double origNoDataA, bool bModifiedMask)
{
  if (!pLercBlob || !pData || !pDataOrig)
    return false;

  const Byte* bytePtr = pLercBlob;
  size_t nBytesRemaining = blobSize;

  bool bHasMask(false);
  Lerc2::HeaderInfo hd;
  if (!Lerc2::GetHeaderInfo(bytePtr, nBytesRemaining, hd, bHasMask))
    return false;

  std::vector<T> arrDec;
  try
  {
    arrDec.assign(hd.nRows * hd.nCols * hd.nDepth, 0);
  }
  catch (...)
  {
    return false;
  }

  BitMask bitMaskDec;
  if (!bitMaskDec.SetSize(hd.nCols, hd.nRows))
    return false;

  bitMaskDec.SetAllInvalid();

  if (!lerc2Verify.Decode(&bytePtr, nBytesRemaining, &arrDec[0], bitMaskDec.Bits()))
    return false;

  // compare decoded bit mask and data array against the input to lerc encode (as after that orig input had the noData value remapped, NaN removed, bit mask altered)
  {
    bool bHasMaskBug(false);
    double maxDelta = 0;

    for (int k = 0, i = 0; i < hd.nRows; i++)
      for (int j = 0; j < hd.nCols; j++, k++)
        if (bitMaskDec.IsValid(k))
        {
          if (pByteMask && !pByteMask[k])
            bHasMaskBug = true;

          for (int n = k * hd.nDepth, m = 0; m < hd.nDepth; m++, n++)
          {
            double d = fabs((double)arrDec[n] - (double)pData[n]);
            if (d > maxDelta)
              maxDelta = d;
          }
        }
        else if (!pByteMask || pByteMask[k])
          bHasMaskBug = true;

    if (bHasMaskBug || maxDelta > maxZErr * 1.1)    // consider float rounding errors
      return false;
  }

  if (!bInputHasNoData && !bModifiedMask)    // any NaN in the input means either modify the mask or replace it by noData value
    return true;

  const bool bIsFltOrDbl = (typeid(T) == typeid(double) || typeid(T) == typeid(float));
  const bool bHaveNoDataVal = (hd.version >= 6 && hd.bPassNoDataValues && hd.nDepth > 1);

  if (bHaveNoDataVal && hd.noDataValOrig != origNoDataA)
    return false;

  if (bHaveNoDataVal && hd.noDataVal != hd.noDataValOrig)
  {
    // remap the noData value from internal to orig
    if (!RemapNoData(&arrDec[0], bitMaskDec, hd))
      return false;
  }

  // compare the final result to orig bit mask and data array as it was handed to the Compress() function
  {
    T noDataOrig = (T)origNoDataA;
    double maxDelta = 0;
    bool bHasBug(false);

    for (int k = 0, i = 0; i < hd.nRows; i++)
      for (int j = 0; j < hd.nCols; j++, k++)
        if (!pByteMaskOrig || pByteMaskOrig[k])
        {
          if (!bitMaskDec.IsValid(k))    // then the orig data values must be noData or NaN
          {
            for (int n = k * hd.nDepth, m = 0; m < hd.nDepth; m++, n++)
            {
              T zOrig = pDataOrig[n];
              bool bIsNoData = (bInputHasNoData && zOrig == noDataOrig) || (bIsFltOrDbl && std::isnan((double)zOrig));

              if (!bIsNoData)
                bHasBug = true;
            }
          }
          else    // valid pixel
          {
            for (int n = k * hd.nDepth, m = 0; m < hd.nDepth; m++, n++)
            {
              T zOrig = pDataOrig[n];
              T z = arrDec[n];

              if (z == zOrig)    // valid value or noData
                continue;

              if (bIsFltOrDbl && std::isnan((double)zOrig))
                zOrig = noDataOrig;

              if (bInputHasNoData && (z == noDataOrig || zOrig == noDataOrig) && (z != zOrig))
                bHasBug = true;

              if (!bHaveNoDataVal || z != noDataOrig)    // is valid
              {
                double d = fabs((double)z - (double)zOrig);
                if (d > maxDelta)
                  maxDelta = d;
              }
            }
          }
        }
        else if (bitMaskDec.IsValid(k))
          bHasBug = true;

    if (bHasBug || maxDelta > maxZErr * 1.1)    // consider float rounding errors
      return false;
  }

  return true;
}

// -------------------------------------------------------------------------- ;

template<class T>
bool Lerc::GetTypeRange(const T, std::pair<double, double>& range)
{
  range.first = 0;

  if (typeid(T) == typeid(Byte))
    range.second = UCHAR_MAX;
  else if (typeid(T) == typeid(unsigned short))
    range.second = USHRT_MAX;
  else if (typeid(T) == typeid(unsigned int) || typeid(T) == typeid(unsigned long))
  range.second = UINT_MAX;

  else if (typeid(T) == typeid(signed char))
    range = std::pair<double, double>(CHAR_MIN, CHAR_MAX);
  else if (typeid(T) == typeid(short))
    range = std::pair<double, double>(SHRT_MIN, SHRT_MAX);
  else if (typeid(T) == typeid(int) || typeid(T) == typeid(long))
    range = std::pair<double, double>(INT_MIN, INT_MAX);
  else
    return false;

  return true;
}

// -------------------------------------------------------------------------- ;

template<class T>
ErrCode Lerc::FilterNoData(std::vector<T>& dataBuffer, std::vector<Byte>& maskBuffer, int nDepth, int nCols, int nRows,
  double& maxZError, bool bPassNoDataValue, double& noDataValue, bool& bModifiedMask, bool& bNeedNoData)
{
  if (nDepth <= 0 || nCols <= 0 || nRows <= 0 || maxZError < 0)
    return ErrCode::WrongParam;

  if ((dataBuffer.size() != (size_t)nDepth * nCols * nRows) || (maskBuffer.size() != (size_t)nCols * nRows))
    return ErrCode::Failed;

  bModifiedMask = false;
  bNeedNoData = false;

  if (!bPassNoDataValue)    // nothing to do
    return ErrCode::Ok;

  std::pair<double, double> typeRange;
  if (!GetTypeRange(dataBuffer[0], typeRange))
    return ErrCode::Failed;

  if (noDataValue < typeRange.first || noDataValue > typeRange.second)
    return ErrCode::WrongParam;

  T origNoData = (T)noDataValue;

  double minVal = DBL_MAX;
  double maxVal = -DBL_MAX;

  // check for noData in valid pixels
  for (int k = 0, i = 0; i < nRows; i++)
  {
    T* rowArr = &(dataBuffer[i * nCols * nDepth]);

    for (int n = 0, j = 0; j < nCols; j++, k++, n += nDepth)
      if (maskBuffer[k])
      {
        int cntInvalid = 0;

        for (int m = 0; m < nDepth; m++)
        {
          T z = rowArr[n + m];

          if (z == origNoData)
            cntInvalid++;
          else if (z < minVal)
            minVal = z;
          else if (z > maxVal)
            maxVal = z;
        }

        if (cntInvalid == nDepth)
        {
          maskBuffer[k] = 0;
          bModifiedMask = true;
        }
        else if (cntInvalid > 0)    // found mix of valid and invalid values at the same pixel
          bNeedNoData = true;
      }
  }

  double maxZErrL = (std::max)(0.5, floor(maxZError));    // same mapping for int types as in Lerc2.cpp
  double dist = floor(maxZErrL);

  // check the orig noData value is far enough away from the valid range
  if ((origNoData >= minVal - dist) && (origNoData <= maxVal + dist))
  {
    maxZError = 0.5;    // fall back to int lossless
    return ErrCode::Ok;
  }

  if (bNeedNoData)
  {
    double minDist = floor(maxZErrL) + 1;
    double remapVal = minVal - minDist;
    T newNoData = origNoData;

    if (remapVal >= typeRange.first)
    {
      newNoData = (T)remapVal;
    }
    else
    {
      maxZErrL = 0.5;    // repeat with int lossless
      remapVal = minVal - 1;

      if (remapVal >= typeRange.first)
      {
        newNoData = (T)remapVal;
      }
      else
      {
        remapVal = maxVal + 1;    // try to map closer to valid range from top

        if ((remapVal <= typeRange.second) && (remapVal < origNoData))
          newNoData = (T)remapVal;
      }
    }

    if (newNoData != origNoData)
    {
      for (int k = 0, i = 0; i < nRows; i++)
      {
        T* rowArr = &(dataBuffer[i * nCols * nDepth]);

        for (int n = 0, j = 0; j < nCols; j++, k++, n += nDepth)
          if (maskBuffer[k])
            for (int m = 0; m < nDepth; m++)
              if (rowArr[n + m] == origNoData)
                rowArr[n + m] = newNoData;
      }

      noDataValue = newNoData;
    }
  }

  if (maxZError != maxZErrL)
    maxZError = maxZErrL;

  return ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;

template<class T>
ErrCode Lerc::FilterNoDataAndNaN(std::vector<T>& dataBuffer, std::vector<Byte>& maskBuffer, int nDepth, int nCols, int nRows,
  double& maxZError, bool bPassNoDataValue, double& noDataValue, bool& bModifiedMask, bool& bNeedNoData, bool& bIsFltDblAllInt)
{
  if (nDepth <= 0 || nCols <= 0 || nRows <= 0 || maxZError < 0)
    return ErrCode::WrongParam;

  if ((dataBuffer.size() != (size_t)nDepth * nCols * nRows) || (maskBuffer.size() != (size_t)nCols * nRows))
    return ErrCode::Failed;

  if (typeid(T) != typeid(double) && typeid(T) != typeid(float))    // only for float or double
    return ErrCode::Failed;

  bModifiedMask = false;
  bNeedNoData = false;
  bIsFltDblAllInt = false;

  bool bHasNoDataValuesLeft = false;
  bool bIsFloat4 = (typeid(T) == typeid(float));
  bool bAllInt = true;
  bool bHasNaN = false;

  T origNoData(0);

  if (bPassNoDataValue)
  {
    if (bIsFloat4 && (noDataValue < -FLT_MAX || noDataValue > FLT_MAX))
      return ErrCode::WrongParam;

    origNoData = (T)noDataValue;
  }
  else
    origNoData = (T)(bIsFloat4 ? -FLT_MAX : -DBL_MAX);

  // only treat int values as int's if within this range, incl orig noData
  const double lowIntLimit = (double)(bIsFloat4 ? -((long)1 << 23) : -((int64_t)1 << 53));
  const double highIntLimit = (double)(bIsFloat4 ? ((long)1 << 23) : ((int64_t)1 << 53));

  double minVal = DBL_MAX;
  double maxVal = -DBL_MAX;
  long cntValidPixels = 0;

  // check for NaN or noData in valid pixels
  for (int k = 0, i = 0; i < nRows; i++)
  {
    T* rowArr = &(dataBuffer[i * nCols * nDepth]);

    for (int n = 0, j = 0; j < nCols; j++, k++, n += nDepth)
      if (maskBuffer[k])
      {
        cntValidPixels++;
        int cntInvalidValues = 0;

        for (int m = 0; m < nDepth; m++)
        {
          T& zVal = rowArr[n + m];

          if (std::isnan((double)zVal))
          {
            bHasNaN = true;
            cntInvalidValues++;

            if (bPassNoDataValue && nDepth > 1)
              zVal = origNoData;    // replace NaN
          }
          else if (bPassNoDataValue && zVal == origNoData)
          {
            cntInvalidValues++;
          }
          else
          {
            if (zVal < minVal)
              minVal = zVal;
            else if (zVal > maxVal)
              maxVal = zVal;

            if (bAllInt && !IsInt(zVal))
              bAllInt = false;
          }
        }

        if (cntInvalidValues == nDepth)
        {
          maskBuffer[k] = 0;
          bModifiedMask = true;
        }
        else if (cntInvalidValues > 0)    // found mix of valid and invalid values at the same pixel
          bHasNoDataValuesLeft = true;
      }
  }

  bNeedNoData = bHasNoDataValuesLeft;

  if (cntValidPixels == 0)
    bAllInt = false;

  if (bHasNaN && nDepth > 1 && bHasNoDataValuesLeft && !bPassNoDataValue)
  {
    return ErrCode::NaN;    // Lerc cannot handle this case, cannot pick a noData value on the tile level
  }

  // now NaN's are gone, either moved to the mask or replaced by noData value

  double maxZErrL = maxZError;

  if (bAllInt)
  {
    bAllInt &= (minVal >= lowIntLimit) && (minVal <= highIntLimit)
      && (maxVal >= lowIntLimit) && (maxVal <= highIntLimit);

    if (bHasNoDataValuesLeft)
      bAllInt &= IsInt(origNoData) && (origNoData >= lowIntLimit) && (origNoData <= highIntLimit);

    if (bAllInt)
      maxZErrL = (std::max)(0.5, floor(maxZError));    // same mapping for int types as in Lerc2.cpp
  }

  bIsFltDblAllInt = bAllInt;

  if (maxZErrL == 0)    // if flt lossless, we are done
    return ErrCode::Ok;

  if (bPassNoDataValue)
  {
    // check the orig noData value is far enough away from the valid range
    double dist = bAllInt ? floor(maxZErrL) : 2 * maxZErrL;

    if ((origNoData >= minVal - dist) && (origNoData <= maxVal + dist))
    {
      maxZError = bAllInt ? 0.5 : 0;    // fall back to lossless
      return ErrCode::Ok;
    }
  }

  if (bHasNoDataValuesLeft)    // try to remap noData to new noData just below the global minimum
  {
    T remapVal = origNoData;
    bool bRemapNoData = FindNewNoDataBelowValidMin(minVal, maxZErrL, bAllInt, lowIntLimit, remapVal);


    //T remapVal = (T)(minVal - max(4 * maxZErrL, 1.0));    // leave some safety margin
    //T lowestVal = (T)(bIsFloat4 ? -FLT_MAX : -DBL_MAX);
    //bool bRemapNoData = (remapVal > lowestVal) && (remapVal < (T)(minVal - 2 * maxZErrL));

    //if (bAllInt)
    //{
    //  bRemapNoData &= IsInt(remapVal) && (remapVal >= lowIntLimit);    // prefer all int over remap
    //}
    //else if (!bRemapNoData)
    //{
    //  // if minVal is a big number
    //  remapVal = (T)((minVal > 0) ? minVal / 2 : minVal * 2);    // try bigger gap
    //  bRemapNoData = (remapVal > lowestVal) && (remapVal < (T)(minVal - 2 * maxZErrL));
    //}


    if (bRemapNoData)
    {
      if (remapVal != origNoData)
      {
        for (int k = 0, i = 0; i < nRows; i++)
        {
          T* rowArr = &(dataBuffer[i * nCols * nDepth]);

          for (int n = 0, j = 0; j < nCols; j++, k++, n += nDepth)
            if (maskBuffer[k])
              for (int m = 0; m < nDepth; m++)
                if (rowArr[n + m] == origNoData)
                  rowArr[n + m] = remapVal;
        }

        noDataValue = remapVal;
      }
    }
    else if ((double)origNoData >= minVal)    // sufficient distance of origNoData to valid range has been checked above
    {
      maxZErrL = bAllInt ? 0.5 : 0;    // need lossless if noData cannot be mapped below valid range
    }
  }

  if (maxZError != maxZErrL)
    maxZError = maxZErrL;

  return ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;

template<class T>
bool Lerc::FindNewNoDataBelowValidMin(double minVal, double maxZErr, bool bAllInt, double lowIntLimit, T& newNoDataVal)
{
  if (bAllInt)
  {
    std::vector<T> noDataCandVec;

    { // collect dist candidates
      std::vector<double> distCandVec = { 4 * maxZErr, 1, 10, 100, 1000, 10000 };    // only int candidates

      for (double dist : distCandVec)
        noDataCandVec.push_back((T)(minVal - dist));

      double candForLargeMinVal = (minVal > 0 ? floor(minVal / 2) : minVal * 2);    // also int
      noDataCandVec.push_back((T)candForLargeMinVal);
    }

    // sort them in descending order
    std::sort(noDataCandVec.begin(), noDataCandVec.end(), std::greater<double>());

    // take the first one that satisfies the condition
    for (T noDataVal : noDataCandVec)
    {
      if ((noDataVal > (T)lowIntLimit) && (noDataVal < (T)(minVal - 2 * maxZErr)) && IsInt(noDataVal))
      {
        newNoDataVal = noDataVal;
        return true;
      }
    }
  }
  else
  {
    std::vector<T> noDataCandVec;

    { // dist candidates
      std::vector<double> distCandVec = { 4 * maxZErr, 0.0001, 0.001, 0.01, 0.1, 1, 10, 100, 1000, 10000 };

      for (double dist : distCandVec)
        noDataCandVec.push_back((T)(minVal - dist));

      double candForLargeMinVal = (minVal > 0 ? minVal / 2 : minVal * 2);
      noDataCandVec.push_back((T)candForLargeMinVal);
    }

    // sort them in descending order
    std::sort(noDataCandVec.begin(), noDataCandVec.end(), std::greater<double>());

    bool bIsFloat4 = (typeid(T) == typeid(float));
    T lowestVal = (T)(bIsFloat4 ? -FLT_MAX : -DBL_MAX);

    // take the first one that satisfies the condition
    for (T noDataVal : noDataCandVec)
    {
      if ((noDataVal > lowestVal) && (noDataVal < (T)(minVal - 2 * maxZErr)))
      {
        newNoDataVal = noDataVal;
        return true;
      }
    }
  }

  return false;
}

// -------------------------------------------------------------------------- ;

