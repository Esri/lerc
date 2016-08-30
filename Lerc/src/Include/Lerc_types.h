
#pragma once

namespace LercNS
{
  enum class ErrCode : int
  {
    Ok = 0,
    Failed,
    WrongParam,
    BufferTooSmall
  };

  enum class DataType : int
  {
    dt_char = 0,
    dt_uchar,
    dt_short,
    dt_ushort,
    dt_int,
    dt_uint,
    dt_float,
    dt_double
  };

  enum class InfoArrOrder : int
  {
    version = 0,
    dataType,
    nCols,
    nRows,
    nBands,
    nValidPixels,
    blobSize
  };

  enum class DataRangeArrOrder : int
  {
    zMin = 0,
    zMax,
    maxZErrUsed
  };

}    // namespace

