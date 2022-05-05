/*
Copyright 2016 - 2022 Esri

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
#include "include/Lerc_c_api.h"
#include "include/Lerc_types.h"
#include "Lerc.h"

USING_NAMESPACE_LERC

// -------------------------------------------------------------------------- ;

lerc_status lerc_computeCompressedSize(const void* pData, unsigned int dataType, int nDepth, int nCols, int nRows, int nBands,
  int nMasks, const unsigned char* pValidBytes, double maxZErr, unsigned int* numBytes)
{
  return lerc_computeCompressedSizeForVersion(pData, -1, dataType, nDepth, nCols, nRows, nBands, nMasks,
    pValidBytes, maxZErr, numBytes);
}

// -------------------------------------------------------------------------- ;

lerc_status lerc_computeCompressedSizeForVersion(const void* pData, int version, unsigned int dataType, int nDepth, int nCols, int nRows, int nBands,
  int nMasks, const unsigned char* pValidBytes, double maxZErr, unsigned int* numBytes)
{
  if (!numBytes)
    return (lerc_status)ErrCode::WrongParam;

  *numBytes = 0;

  if (!pData || dataType >= Lerc::DT_Undefined || nDepth <= 0 || nCols <= 0 || nRows <= 0 || nBands <= 0 || maxZErr < 0)
    return (lerc_status)ErrCode::WrongParam;

  if (!(nMasks == 0 || nMasks == 1 || nMasks == nBands) || (nMasks > 0 && !pValidBytes))
    return (lerc_status)ErrCode::WrongParam;

  Lerc::DataType dt = (Lerc::DataType)dataType;
  return (lerc_status)Lerc::ComputeCompressedSize(pData, version, dt, nDepth, nCols, nRows, nBands, nMasks,
    pValidBytes, maxZErr, *numBytes, nullptr, nullptr);
}

// -------------------------------------------------------------------------- ;

lerc_status lerc_encode(const void* pData, unsigned int dataType, int nDepth, int nCols, int nRows, int nBands,
  int nMasks, const unsigned char* pValidBytes, double maxZErr, unsigned char* pOutBuffer, unsigned int outBufferSize,
  unsigned int* nBytesWritten)
{
  return lerc_encodeForVersion(pData, -1, dataType, nDepth, nCols, nRows, nBands, nMasks, pValidBytes,
    maxZErr, pOutBuffer, outBufferSize, nBytesWritten);
}

// -------------------------------------------------------------------------- ;

lerc_status lerc_encodeForVersion(const void* pData, int version, unsigned int dataType, int nDepth, int nCols, int nRows, int nBands, 
  int nMasks, const unsigned char* pValidBytes, double maxZErr, unsigned char* pOutBuffer, unsigned int outBufferSize,
  unsigned int* nBytesWritten)
{
  if (!nBytesWritten)
    return (lerc_status)ErrCode::WrongParam;

  *nBytesWritten = 0;

  if (!pData || dataType >= Lerc::DT_Undefined || nDepth <= 0 || nCols <= 0 || nRows <= 0 || nBands <= 0 || maxZErr < 0 || !pOutBuffer || !outBufferSize)
    return (lerc_status)ErrCode::WrongParam;

  if (!(nMasks == 0 || nMasks == 1 || nMasks == nBands) || (nMasks > 0 && !pValidBytes))
    return (lerc_status)ErrCode::WrongParam;

  Lerc::DataType dt = (Lerc::DataType)dataType;
  return (lerc_status)Lerc::Encode(pData, version, dt, nDepth, nCols, nRows, nBands, nMasks, pValidBytes,
    maxZErr, pOutBuffer, outBufferSize, *nBytesWritten, nullptr, nullptr);
}

// -------------------------------------------------------------------------- ;

lerc_status lerc_getBlobInfo(const unsigned char* pLercBlob, unsigned int blobSize, 
  unsigned int* infoArray, double* dataRangeArray, int infoArraySize, int dataRangeArraySize)
{
  if (!pLercBlob || !blobSize || (!infoArray && !dataRangeArray) || ((infoArraySize <= 0) && (dataRangeArraySize <= 0)))
    return (lerc_status)ErrCode::WrongParam;

  Lerc::LercInfo lercInfo;
  ErrCode errCode = Lerc::GetLercInfo(pLercBlob, blobSize, lercInfo);
  if (errCode != ErrCode::Ok)
    return (lerc_status)errCode;

  if (infoArray)
  {
    int i = 0, ias = infoArraySize;

    if (ias > 0)
      memset(infoArray, 0, ias * sizeof(infoArray[0]));

    if (i < ias)
      infoArray[i++] = (unsigned int)lercInfo.version;
    if (i < ias)
      infoArray[i++] = (unsigned int)lercInfo.dt;
    if (i < ias)
      infoArray[i++] = (unsigned int)lercInfo.nDepth;
    if (i < ias)
      infoArray[i++] = (unsigned int)lercInfo.nCols;
    if (i < ias)
      infoArray[i++] = (unsigned int)lercInfo.nRows;
    if (i < ias)
      infoArray[i++] = (unsigned int)lercInfo.nBands;
    if (i < ias)
      infoArray[i++] = (unsigned int)lercInfo.numValidPixel;
    if (i < ias)
      infoArray[i++] = (unsigned int)lercInfo.blobSize;
    if (i < ias)
      infoArray[i++] = (unsigned int)lercInfo.nMasks;
    if (i < ias)
      infoArray[i++] = (unsigned int)lercInfo.nDepth;
    if (i < ias)
      infoArray[i++] = (unsigned int)lercInfo.nUsesNoDataValue;
  }

  if (dataRangeArray)
  {
    int i = 0, dras = dataRangeArraySize;

    if (dras > 0)
      memset(dataRangeArray, 0, dras * sizeof(dataRangeArray[0]));

    // for nDepth > 1, and mix of valid and invalid values at same pixel, better reset min / max to -1
    // than return min or max values that contain noData (to be fixed in next codec version)

    bool bUsesNoData = (lercInfo.nDepth > 1) && (lercInfo.nUsesNoDataValue > 0);

    if (i < dras)
      dataRangeArray[i++] = !bUsesNoData ? lercInfo.zMin : -1;
    if (i < dras)
      dataRangeArray[i++] = !bUsesNoData ? lercInfo.zMax : -1;
    if (i < dras)
      dataRangeArray[i++] = lercInfo.maxZError;
  }

  return (lerc_status)ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;

lerc_status lerc_getDataRanges(const unsigned char* pLercBlob, unsigned int blobSize,
  int nDepth, int nBands, double* pMins, double* pMaxs)
{
  if (!pLercBlob || !blobSize || !pMins || !pMaxs || nDepth <= 0 || nBands <= 0)
    return (lerc_status)ErrCode::WrongParam;

  Lerc::LercInfo lercInfo;
  ErrCode errCode = Lerc::GetLercInfo(pLercBlob, blobSize, lercInfo, pMins, pMaxs, (size_t)nDepth * (size_t)nBands);
  if (errCode != ErrCode::Ok)
    return (lerc_status)errCode;

  return (lerc_status)ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;

lerc_status lerc_decode(const unsigned char* pLercBlob, unsigned int blobSize, int nMasks,
  unsigned char* pValidBytes, int nDepth, int nCols, int nRows, int nBands, unsigned int dataType, void* pData)
{
  return lerc_decode_4D(pLercBlob, blobSize, nMasks, pValidBytes,
    nDepth, nCols, nRows, nBands, dataType, pData, nullptr, nullptr);
}
// -------------------------------------------------------------------------- ;

lerc_status lerc_decodeToDouble(const unsigned char* pLercBlob, unsigned int blobSize, int nMasks,
  unsigned char* pValidBytes, int nDepth, int nCols, int nRows, int nBands, double* pData)
{
  return lerc_decodeToDouble_4D(pLercBlob, blobSize, nMasks, pValidBytes,
    nDepth, nCols, nRows, nBands, pData, nullptr, nullptr);
}

// -------------------------------------------------------------------------- ;
// -------------------------------------------------------------------------- ;

lerc_status lerc_computeCompressedSize_4D(const void* pData, unsigned int dataType, int nDepth, int nCols, int nRows, int nBands,
  int nMasks, const unsigned char* pValidBytes, double maxZErr, unsigned int* numBytes, const unsigned char* pUsesNoData, const double* noDataValues)
{
  if (!numBytes)
    return (lerc_status)ErrCode::WrongParam;

  *numBytes = 0;

  if (!pData || dataType >= Lerc::DT_Undefined || nDepth <= 0 || nCols <= 0 || nRows <= 0 || nBands <= 0 || maxZErr < 0)
    return (lerc_status)ErrCode::WrongParam;

  if (!(nMasks == 0 || nMasks == 1 || nMasks == nBands) || (nMasks > 0 && !pValidBytes))
    return (lerc_status)ErrCode::WrongParam;

  Lerc::DataType dt = (Lerc::DataType)dataType;
  return (lerc_status)Lerc::ComputeCompressedSize(pData, -1, dt, nDepth, nCols, nRows, nBands, nMasks,
    pValidBytes, maxZErr, *numBytes, pUsesNoData, noDataValues);
}

// -------------------------------------------------------------------------- ;

lerc_status lerc_encode_4D(const void* pData, unsigned int dataType, int nDepth, int nCols, int nRows, int nBands,
  int nMasks, const unsigned char* pValidBytes, double maxZErr, unsigned char* pOutBuffer, unsigned int outBufferSize,
  unsigned int* nBytesWritten, const unsigned char* pUsesNoData, const double* noDataValues)
{
  if (!nBytesWritten)
    return (lerc_status)ErrCode::WrongParam;

  *nBytesWritten = 0;

  if (!pData || dataType >= Lerc::DT_Undefined || nDepth <= 0 || nCols <= 0 || nRows <= 0 || nBands <= 0 || maxZErr < 0 || !pOutBuffer || !outBufferSize)
    return (lerc_status)ErrCode::WrongParam;

  if (!(nMasks == 0 || nMasks == 1 || nMasks == nBands) || (nMasks > 0 && !pValidBytes))
    return (lerc_status)ErrCode::WrongParam;

  Lerc::DataType dt = (Lerc::DataType)dataType;
  return (lerc_status)Lerc::Encode(pData, -1, dt, nDepth, nCols, nRows, nBands, nMasks, pValidBytes,
    maxZErr, pOutBuffer, outBufferSize, *nBytesWritten, pUsesNoData, noDataValues);
}

// -------------------------------------------------------------------------- ;

lerc_status lerc_decode_4D(const unsigned char* pLercBlob, unsigned int blobSize, int nMasks,
  unsigned char* pValidBytes, int nDepth, int nCols, int nRows, int nBands, unsigned int dataType, void* pData,
  unsigned char* pUsesNoData, double* noDataValues)
{
  if (!pLercBlob || !blobSize || !pData || dataType >= Lerc::DT_Undefined || nDepth <= 0 || nCols <= 0 || nRows <= 0 || nBands <= 0)
    return (lerc_status)ErrCode::WrongParam;

  if (!(nMasks == 0 || nMasks == 1 || nMasks == nBands) || (nMasks > 0 && !pValidBytes))
    return (lerc_status)ErrCode::WrongParam;

  Lerc::DataType dt = (Lerc::DataType)dataType;

  return (lerc_status)Lerc::Decode(pLercBlob, blobSize, nMasks, pValidBytes, nDepth, nCols, nRows, nBands, dt, pData, pUsesNoData, noDataValues);
}

// -------------------------------------------------------------------------- ;

lerc_status lerc_decodeToDouble_4D(const unsigned char* pLercBlob, unsigned int blobSize, int nMasks,
  unsigned char* pValidBytes, int nDepth, int nCols, int nRows, int nBands, double* pData,
  unsigned char* pUsesNoData, double* noDataValues)
{
  if (!pLercBlob || !blobSize || !pData || nDepth <= 0 || nCols <= 0 || nRows <= 0 || nBands <= 0)
    return (lerc_status)ErrCode::WrongParam;

  if (!(nMasks == 0 || nMasks == 1 || nMasks == nBands) || (nMasks > 0 && !pValidBytes))
    return (lerc_status)ErrCode::WrongParam;

  Lerc::LercInfo lercInfo;
  ErrCode errCode;
  if ((errCode = Lerc::GetLercInfo(pLercBlob, blobSize, lercInfo)) != ErrCode::Ok)
    return (lerc_status)errCode;

  Lerc::DataType dt = lercInfo.dt;
  if (dt > Lerc::DT_Double)
    return (lerc_status)ErrCode::Failed;

  if (dt == Lerc::DT_Double)
  {
    if ((errCode = Lerc::Decode(pLercBlob, blobSize, nMasks, pValidBytes,
      nDepth, nCols, nRows, nBands, dt, pData, pUsesNoData, noDataValues)) != ErrCode::Ok)
    {
      return (lerc_status)errCode;
    }
  }
  else
  {
    // use the buffer passed for in place decode and convert
    int sizeofDt[] = { 1, 1, 2, 2, 4, 4, 4, 8 };
    size_t nDataValues = nDepth * nCols * nRows * nBands;
    void* ptrDec = (Byte*)pData + nDataValues * (sizeof(double) - sizeofDt[dt]);

    if ((errCode = Lerc::Decode(pLercBlob, blobSize, nMasks, pValidBytes,
      nDepth, nCols, nRows, nBands, dt, ptrDec, pUsesNoData, noDataValues)) != ErrCode::Ok)
    {
      return (lerc_status)errCode;
    }

    if ((errCode = Lerc::ConvertToDouble(ptrDec, dt, nDataValues, pData)) != ErrCode::Ok)
      return (lerc_status)errCode;
  }

  return (lerc_status)ErrCode::Ok;
}

// -------------------------------------------------------------------------- ;

