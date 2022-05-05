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

#include "fpl_UnitTypes.h"
#include <assert.h>
#include <cstring>
#include <stdint.h>

USING_NAMESPACE_LERC

#define FLT_TYPES_ONLY  // comment out this define to enable all types

static const uint32_t FLT_MANT_MASK = 0x007FFFFFU;
static const uint32_t FLT_9BIT_MASK = 0xFF800000U;

#define FLT_BIT_23    (1 << 22) //0x00400000U

uint32_t moveBits2Front(const uint32_t a)
{
  uint32_t ret = a & FLT_MANT_MASK;

  uint32_t ae = ((a & FLT_9BIT_MASK) >> 23) & 0xFF;

  uint32_t as = ((a & 0x80000000) >> 31) & 0x01;

  ret |= (ae << 24);
  ret |= (as << 23);

  return ret;
}

uint32_t undo_moveBits2Front(const uint32_t a)
{
  uint32_t ret = a & FLT_MANT_MASK;

  uint32_t ae = ((a & FLT_9BIT_MASK) >> 24) & 0xFF;

  uint32_t as = (a >> 23) & 0x01;

  ret |= (ae << 23);
  ret |= (as << 31);

  return ret;
}

void UnitTypes::doFloatTransform(uint32_t* pData, const size_t iCnt)
{
  for (size_t i = 0; i < iCnt; i++)
  {
    pData[i] = moveBits2Front(pData[i]);
  }
}

void UnitTypes::undoFloatTransform(uint32_t* pData, const size_t iCnt)
{
  for (size_t i = 0; i < iCnt; i++)
  {
    pData[i] = undo_moveBits2Front(pData[i]);
  }
}

uint32_t SUB32_BIT_FLT(const uint32_t& a, const uint32_t& b)
{
  uint32_t ret = 0;

  const bool no_exp_delta = false;

  ret = ((a - b) & FLT_MANT_MASK);

  uint32_t ae = ((a & FLT_9BIT_MASK) >> 23) & 0x1FF;
  uint32_t be = no_exp_delta ? 0 : ((b & FLT_9BIT_MASK) >> 23) & 0x1FF;

  ret |= (((ae - be) & 0x1FF) << 23);

  return ret;
}

uint32_t ADD32_BIT_FLT(const uint32_t& a, const uint32_t& b)
{
  uint32_t ret = 0;

  const bool no_exp_delta = false;

  ret = ((a + b) & FLT_MANT_MASK);

  uint32_t ae = ((a & FLT_9BIT_MASK) >> 23) & 0x1FF;
  uint32_t be = no_exp_delta ? 0 : ((b & FLT_9BIT_MASK) >> 23) & 0x1FF;

  ret |= (((ae + be) & 0x1FF) << 23);

  return ret;
}

static const uint64_t DBL_MANT_MASK = 0x000FFFFFFFFFFFFFULL;
static const uint64_t DBL_12BIT_MASK = 0xFFF0000000000000ULL;


uint64_t SUB64_BIT_DBL(const uint64_t& a, const uint64_t& b)
{
  uint64_t ret = 0;

  const bool no_exp_delta = false;

  uint64_t am = a & DBL_MANT_MASK;
  uint64_t bm = b & DBL_MANT_MASK;

  ret = ((am - bm) & DBL_MANT_MASK);

  uint64_t ae = ((a & DBL_12BIT_MASK) >> 52) & 0xFFF;
  uint64_t be = no_exp_delta ? 0 : ((b & DBL_12BIT_MASK) >> 52) & 0xFFF;

  ret |= (((ae - be) & 0xFFF) << 52);

  return ret;
}

uint64_t ADD64_BIT_DBL(const uint64_t& a, const uint64_t& b)
{
  uint64_t ret = 0;

  const bool no_exp_delta = false;

  uint64_t am = a & DBL_MANT_MASK;
  uint64_t bm = b & DBL_MANT_MASK;

  ret = ((am + bm) & DBL_MANT_MASK);

  uint64_t ae = ((a & DBL_12BIT_MASK) >> 52) & 0xFFF;
  uint64_t be = no_exp_delta ? 0 : ((b & DBL_12BIT_MASK) >> 52) & 0xFFF;

  ret |= (((ae + be) & 0xFFF) << 52);

  return ret;
}

size_t UnitTypes::size (const UnitType type)
{
    switch (type)
    {
#ifndef FLT_TYPES_ONLY
        case UNIT_TYPE_BYTE:
            return 1;
        case UNIT_TYPE_SHORT:
            return 2;
        case UNIT_TYPE_LONG:
            return 4;
        case UNIT_TYPE_64_BIT:
            return 8;
#endif
        case UNIT_TYPE_FLOAT:
            return 4;
        case UNIT_TYPE_DOUBLE:
            assert(sizeof(double) == 8);
            return 8;
        default:
            assert(0);
            return 0;
    }
}

/////////////////////////////////////////////////////////

UnitType UnitTypes::type (int iBytes, bool bFloatType)
{
    if (iBytes == 1)
        return UNIT_TYPE_BYTE;

    else if (iBytes == 2)
        return UNIT_TYPE_SHORT;

    else if (iBytes == 4)
    {
        if (bFloatType)
            return UNIT_TYPE_FLOAT;
        return UNIT_TYPE_LONG;
    }

    else if (iBytes == 8)
    {
        if (bFloatType)
            return UNIT_TYPE_DOUBLE;

        return UNIT_TYPE_64_BIT;
    }

    assert (0);

    return UNIT_TYPE_UNKNOWN;
}

///////////////////////////////////////////////////////////////////////////////////////////////

unsigned char UnitTypes::unitCode (const UnitType type)
{
    if (type >= UNIT_TYPE_BYTE && type <= UNIT_TYPE_DOUBLE)
        return (unsigned char)type;

    assert (0);

    return 0;
}

///////////////////////////////////////////////////////////////////////////////////////////////
void setDerivativeFloat (uint32_t * pData, const size_t count, const int level, const int start_level)
{
    for (int l = start_level; l <= level; l++)
    {
        for (int i = (int)count - 1; i >= l; i--)
        {
            pData[i] = SUB32_BIT_FLT (pData[i], pData[i - 1]);
        }
    }
}

void setDerivativeDouble (uint64_t * pData, const size_t count, const int nLevel, const int start_level)
{
    for (int l = start_level; l <= nLevel; l++)
    {
        for (int i = (int)count - 1; i >= l; i--)
        {
            pData[i] = SUB64_BIT_DBL (pData[i], pData[i - 1]);
        }
    }
}

template <typename T>
void setDerivativeGeneric (T * pData, const size_t count, const int nLevel, const int start_level)
{
    for (int l = start_level; l <= nLevel; l++)
    {
        for (int i = (int)count - 1; i >= l; i--)
        {
           pData[i] = pData[i] - pData[i - 1];
        }
    }
}


void UnitTypes::setDerivative (const UnitType type, void *pData, const size_t nCount, const int nLevel, const int start_level)
{
    assert (nCount > 0);

    if (nLevel == 0) return;

    if (type == UNIT_TYPE_FLOAT )
    {
        setDerivativeFloat ((uint32_t *)pData, nCount, nLevel, start_level);
    }
#ifndef FLT_TYPES_ONLY
    else if (type == UNIT_TYPE_BYTE)
    {
        setDerivativeGeneric<uint8_t> ( (uint8_t *)pData, count, level, start_level);
    }
    else if (type == UNIT_TYPE_SHORT)
    {
        setDerivativeGeneric<uint16_t> ( (uint16_t *)pData, count, level, start_level);
    }
    else if (type == UNIT_TYPE_LONG)
    {
        setDerivativeGeneric<uint32_t> ( (uint32_t *)pData, count, level, start_level);
    }
    else if (type == UNIT_TYPE_64_BIT)
    {
        setDerivativeGeneric<uint64_t> ( (uint64_t *)pData, count, level, start_level);
    }
#endif
    else if (type == UNIT_TYPE_DOUBLE)
    {
      //  assert (0); // TODO
        setDerivativeDouble ((uint64_t *)pData, nCount, nLevel, start_level);
    }
    else
    {
        //printf ("Unknown unit type: %d\n", type);
        assert (0);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////

void setRowsDerivativeFloat (uint32_t *pData, const size_t nCols, const size_t nRows, const int nLevel, int phase)
{
    assert (nCols > 0 && nRows > 0);

    assert (nLevel >= 2) ;

    uint32_t * row_data = pData;

    int start_level = 1;
    int end_level = nLevel;

    if (phase == 2) start_level = 2;
    if (phase == 1) end_level = 1;

    for (size_t iRow = 0; iRow < nRows; iRow++)
    {
        for (int l = start_level; l <= end_level; l++)
        {
            for (int i = (int)nCols - 1; i >= l; i--)
            {
                row_data [i] = SUB32_BIT_FLT (row_data[i], row_data[i - 1]);
            }
        }

        row_data += nCols;
    }
}

void setRowsDerivativeDouble (uint64_t *pData, const size_t nCols, const size_t nRows, const int nLevel, int phase)
{
    assert (nCols > 0 && nRows > 0);

    assert (nLevel >= 2) ;

    uint64_t * row_data = pData;

    int start_level = 1;
    int end_level = nLevel;

    if (phase == 2) start_level = 2;
    if (phase == 1) end_level = 1;

    for (size_t iRow = 0; iRow < nRows; iRow++)
    {
        for (int l = start_level; l <= end_level; l++)
        {
            for (int i = (int)nCols - 1; i >= l; i--)
            {
                row_data [i] = SUB64_BIT_DBL (row_data[i], row_data[i - 1]);
            }
        }

        row_data += nCols;
    }
}


template <typename T>
void setRowsDerivativeGeneric (T * pData, const size_t nCols, const size_t nRows, const int nLevel, int phase)
{
    assert (nCols > 0 && nRows > 0);

    assert (nLevel >= 2) ;

    T * row_data = pData;

    int start_level = 1;
    int end_level = nLevel;

    if (phase == 2) start_level = 2;
    if (phase == 1) end_level = 1;

    for (size_t iRow = 0; iRow < nRows; iRow++)
    {
        for (int l = start_level; l <= end_level; l++)
        {
            for (int i = (int)nCols - 1; i >= l; i--)
            {
                row_data [i] = row_data [i] - row_data [i - 1];
            }
        }

        row_data += nCols;
    }
}


///////////////////////////////////////////////////////////////////////////////////////////////////

// phase 0 : start = 1, end = level
// phase 1 : start = 1, end = 1
// phase 2 : start = 2, end = 2

void UnitTypes::setRowsDerivative (const UnitType type, void *pData, const size_t nCols, const size_t nRows, const int nLevel, int phase)
{
    assert (nCols > 0 && nRows > 0);

    assert (nLevel >= 2) ;

    if (type == UNIT_TYPE_FLOAT )
    {
        setRowsDerivativeFloat ((uint32_t *)pData, nCols, nRows, nLevel, phase);
    }
#ifndef FLT_TYPES_ONLY
    else if (type == UNIT_TYPE_BYTE)
    {
        setRowsDerivativeGeneric<uint8_t> ( (uint8_t *)pData, nCols, nRows, nLevel, phase);
    }
    else if (type == UNIT_TYPE_SHORT)
    {
        setRowsDerivativeGeneric<uint16_t> ( (uint16_t *)pData, nCols, nRows, nLevel, phase);
    }
    else if (type == UNIT_TYPE_LONG)
    {
        setRowsDerivativeGeneric<uint32_t> ( (uint32_t *)pData, nCols, nRows, nLevel, phase);
    }
    else if (type == UNIT_TYPE_64_BIT)
    {
        setRowsDerivativeGeneric<uint64_t> ( (uint64_t *)pData, nCols, nRows, nLevel, phase);
    }
#endif
    else if (type == UNIT_TYPE_DOUBLE)
    {
        setRowsDerivativeDouble ( (uint64_t *)pData, nCols, nRows, nLevel, phase);
    }
    else
    {
        assert (0);
    }

}

////////////////////////////////////////////////////////////////////////////////////////////////

void setCrossDerivativeFloat (uint32_t *pData, const size_t nCols, const size_t nRows, const int nLevel, int phase)
{
    assert (nCols > 0 && nRows > 0);

    assert (nLevel >= 2) ;

    uint32_t * row_data = pData;

    if (phase == 0 || phase == 1)
    {
        for (size_t iRow = 0; iRow < nRows; iRow++)
        {
            for (int i = (int)nCols - 1; i >= 1; i--)
            {
                row_data[i] = SUB32_BIT_FLT (row_data[i], row_data[i - 1]);
            }

            row_data += nCols;
        }
    }

    row_data = pData;

    if (phase == 0 || phase == 2)
    {
        for (size_t iCol = 0; iCol < nCols; iCol++)
        {
            size_t iShift = (nRows - 1) * nCols;

            for (int i = (int)nRows - 1; i >= 1; i--)
            {
                row_data[iShift] = SUB32_BIT_FLT (row_data[iShift], row_data[iShift - nCols]);

                iShift -= nCols;
            }

            row_data += 1;
        }
    }
}

void setCrossDerivativeDouble (uint64_t *pData, const size_t nCols, const size_t nRows, const int nLevel, int phase)
{
    assert (nCols > 0 && nRows > 0);

    assert (nLevel >= 2) ;

    uint64_t * row_data = pData;

    if (phase == 0 || phase == 1)
    {
        for (size_t iRow = 0; iRow < nRows; iRow++)
        {
            for (int i = (int)nCols - 1; i >= 1; i--)
            {
                row_data[i] = SUB64_BIT_DBL (row_data[i], row_data[i - 1]);
            }

            row_data += nCols;
        }
    }

    row_data = pData;

    if (phase == 0 || phase == 2)
    {
        for (size_t iCol = 0; iCol < nCols; iCol++)
        {
            size_t iShift = (nRows - 1) * nCols;

            for (int i = (int)nRows - 1; i >= 1; i--)
            {
                row_data[iShift] = SUB64_BIT_DBL (row_data[iShift], row_data[iShift - nCols]);

                iShift -= nCols;
            }

            row_data += 1;
        }
    }
}


template <typename T>
void setCrossDerivativeGeneric (T *pData, const size_t nCols, const size_t nRows, const int nLevel, int phase)
{
    assert (nCols > 0 && nRows > 0);

    assert (nLevel >= 2) ;

    T * row_data = pData;

    if (phase == 0 || phase == 1)
    {
        for (size_t iRow = 0; iRow < nRows; iRow++)
        {
            for (int i = (int)nCols - 1; i >= 1; i--)
            {
                row_data [i] = row_data [i] - row_data [i - 1];
            }

            row_data += nCols;
        }
    }

    row_data = pData;

    if (phase == 0 || phase == 2)
    {
        for (size_t iCol = 0; iCol < nCols; iCol++)
        {
            size_t iShift = (nRows - 1) * nCols;

            for (int i = (int)nRows - 1; i >= 1; i--)
            {
                row_data [iShift] = row_data [iShift] - row_data [iShift - nCols];

                iShift -= nCols;
            }

            row_data += 1;
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////


void UnitTypes::setCrossDerivative (const UnitType type, void *pData, const size_t nCols, const size_t nRows, const int nLevel, int phase)
{
    assert (nCols > 0 && nRows > 0);

    assert (nLevel >= 2) ;

    if (type == UNIT_TYPE_FLOAT )
    {
        setCrossDerivativeFloat ((uint32_t *)pData, nCols, nRows, nLevel, phase);
    }
#ifndef FLT_TYPES_ONLY
    else if (type == UNIT_TYPE_BYTE)
    {
        setCrossDerivativeGeneric<uint8_t> ( (uint8_t *)pData, nCols, nRows, nLevel, phase);
    }
    else if (type == UNIT_TYPE_SHORT)
    {
        setCrossDerivativeGeneric<uint16_t> ( (uint16_t *)pData, nCols, nRows, nLevel, phase);
    }
    else if (type == UNIT_TYPE_LONG)
    {
        setCrossDerivativeGeneric<uint32_t> ( (uint32_t *)pData, nCols, nRows, nLevel, phase);
    }
    else if (type == UNIT_TYPE_64_BIT)
    {
        setCrossDerivativeGeneric<uint64_t> ( (uint64_t *)pData, nCols, nRows, nLevel, phase);
    }
#endif

    else if (type == UNIT_TYPE_DOUBLE)
    {
        setCrossDerivativeDouble ((uint64_t *)pData, nCols, nRows, nLevel, phase);
    }
    else
    {
        assert (0);
    }

}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////

void UnitTypes::setBlockDerivative (const UnitType type, void *pData, const size_t nCols, const size_t nRows, const int level, const int start_level)
{
     if (level == 0) return;

    int phase = -1;

    if (start_level == 1 && level == 2)
        phase = 0;
    else if (start_level == 1 && level == 1)
        phase = 1;
    else if (start_level == 2 && level == 2)
        phase = 2;

    assert (phase >= 0);

    setRowsDerivative (type, pData, nCols, nRows, 2, phase);
}

//////////////////////////////////////////////////////////////////////////////

void restoreBlockSequenceFloat (const int nDelta, uint32_t *pData, const size_t nCols, const size_t nRows)
{
    uint32_t * row_data = pData;

    if (true)
    {
        if (nDelta == 2)
        {
            for (size_t iRow = 0; iRow < nRows; iRow++)
            {
                for (size_t i = 2; i < nCols; i++)
                {
                    row_data [i] = ADD32_BIT_FLT (row_data [i], row_data [i - 1] );
                }

                row_data += nCols;
            }
        }

        if (nDelta > 0)
        {
            row_data = pData;

            for (size_t iRow = 0; iRow < nRows; iRow++)
            {
                for (size_t i = 1; i < nCols; i++)
                {
                    row_data [i] = ADD32_BIT_FLT (row_data [i], row_data [i - 1] );
                }

                row_data += nCols;
            }
        }
    }
}

void restoreBlockSequenceDouble (const int nDelta, uint64_t *pData, const size_t nCols, const size_t nRows)
{
    uint64_t * row_data = pData;

    if (true)
    {
        if (nDelta == 2)
        {
            for (size_t iRow = 0; iRow < nRows; iRow++)
            {
                for (size_t i = 2; i < nCols; i++)
                {
                    row_data [i] = ADD64_BIT_DBL (row_data [i], row_data [i - 1] );
                }

                row_data += nCols;
            }
        }

        if (nDelta > 0)
        {
            row_data = pData;

            for (size_t iRow = 0; iRow < nRows; iRow++)
            {
                for (size_t i = 1; i < nCols; i++)
                {
                    row_data [i] = ADD64_BIT_DBL (row_data [i], row_data [i - 1] );
                }

                row_data += nCols;
            }
        }
    }
}


template<typename T>
void restoreBlockSequenceGeneric (const int nDelta, void *pData, const size_t nCols, const size_t nRows)
{
    T * row_data = (T *)pData;

    if (true)
    {
        if (nDelta == 2)
        {
            for (size_t iRow = 0; iRow < nRows; iRow++)
            {
                for (size_t i = 2; i < nCols; i++)
                {
                    row_data [i] = row_data [i] + row_data [i - 1];
                }

                row_data += nCols;
            }
        }

        if (nDelta > 0)
        {
            row_data = (T *)pData;

            for (size_t iRow = 0; iRow < nRows; iRow++)
            {
                for (size_t i = 1; i < nCols; i++)
                {
                    row_data [i] = row_data [i] + row_data [i - 1];
                }

                row_data += nCols;
            }
        }
    }
}


void UnitTypes::restoreBlockSequence (const int delta, void *pData, const size_t nCols, const size_t nRows, const UnitType type)
{
    if (delta == 0) return;

    if (type == UNIT_TYPE_FLOAT )
    {
        restoreBlockSequenceFloat (delta, (uint32_t *)pData, nCols, nRows);
    }
#ifndef FLT_TYPES_ONLY
    else if (type == UNIT_TYPE_BYTE)
    {
        restoreBlockSequenceGeneric<uint8_t> (delta, pData, nCols, nRows);
    }
    else if (type == UNIT_TYPE_SHORT)
    {
        restoreBlockSequenceGeneric<uint16_t> (delta, pData, nCols, nRows);
    }
    else if (type == UNIT_TYPE_LONG)
    {
        restoreBlockSequenceGeneric<uint32_t> (delta, pData, nCols, nRows);
    }
    else if (type == UNIT_TYPE_64_BIT)
    {
        restoreBlockSequenceGeneric<uint64_t> (delta, pData, nCols, nRows);
    }
#endif
    else if (type == UNIT_TYPE_DOUBLE)
    {
        restoreBlockSequenceDouble (delta, (uint64_t *)pData, nCols, nRows);
    }
    else
    {
        assert (0);
    }
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void restoreCrossBytesFloat (const int delta, uint32_t *pData, const size_t nCols, const size_t nRows)
{
    uint32_t * row_data = pData;

    if (true)
    {
        if (delta == 2)
        {
            for (size_t iCol = 0; iCol < nCols; iCol++)
            {
                size_t iShift = nCols;

                for (size_t i = 1; i < nRows; i++)
                {
                    row_data [iShift] = ADD32_BIT_FLT (row_data [iShift], row_data [iShift - nCols] );

                    iShift += nCols;
                }

                row_data += 1;
            }
        }

        row_data = pData;

        for (size_t iRow = 0; iRow < nRows; iRow++)
        {
            for (size_t i = 1; i < nCols; i++)
            {
                row_data [i] = ADD32_BIT_FLT (row_data [i], row_data [i - 1] );
            }

            row_data += nCols;
        }
    }

}

void restoreCrossBytesDouble (const int delta, uint64_t *pData, const size_t nCols, const size_t nRows)
{
    uint64_t * row_data = pData;

    if (true)
    {
        if (delta == 2)
        {
            for (size_t col = 0; col < nCols; col++)
            {
                size_t iShift = nCols;

                for (size_t i = 1; i < nRows; i++)
                {
                    row_data [iShift] = ADD64_BIT_DBL (row_data [iShift], row_data [iShift - nCols] );

                    iShift += nCols;
                }

                row_data += 1;
            }
        }

        row_data = pData;

        for (size_t iRow = 0; iRow < nRows; iRow++)
        {
            for (size_t i = 1; i < nCols; i++)
            {
                row_data [i] = ADD64_BIT_DBL (row_data [i], row_data [i - 1] );
            }

            row_data += nCols;
        }
    }
}


template<typename T>
void restoreCrossBytesGeneric (const int nDelta, void *pData, const size_t nCols, const size_t nRows)
{
    T * row_data = (T *)pData;

    if (true)
    {
        if (nDelta == 2)
        {
            for (size_t iCol = 0; iCol < nCols; iCol++)
            {
                size_t iShift = nCols;

                for (size_t i = 1; i < nRows; i++)
                {
                    row_data [iShift] = row_data [iShift] + row_data [iShift - nCols];

                    iShift += nCols;
                }

                row_data += 1;
            }
        }

        row_data = (T *)pData;

        for (size_t iRow = 0; iRow < nRows; iRow++)
        {
            for (size_t i = 1; i < nCols; i++)
            {
                row_data [i] = row_data [i] + row_data [i - 1];
            }

            row_data += nCols;
        }
    }

}

void UnitTypes::restoreCrossBytes (const int delta, void *pData, const size_t nCols, const size_t nRows, const UnitType type)
{
    if (delta == 0) return;

    if (type == UNIT_TYPE_FLOAT )
    {
        restoreCrossBytesFloat (delta, (uint32_t *)pData, nCols, nRows);
    }
#ifndef FLT_TYPES_ONLY
    else if (type == UNIT_TYPE_BYTE)
    {
        restoreCrossBytesGeneric<uint8_t> (delta, pData, nCols, nRows);
    }
    else if (type == UNIT_TYPE_SHORT)
    {
        restoreCrossBytesGeneric<uint16_t> (delta, pData, nCols, nRows);
    }
    else if (type == UNIT_TYPE_LONG)
    {
        restoreCrossBytesGeneric<uint32_t> (delta, pData, nCols, nRows);
    }
    else if (type == UNIT_TYPE_64_BIT)
    {
        restoreCrossBytesGeneric<uint64_t> (delta, pData, nCols, nRows);
    }
#endif
    else if (type == UNIT_TYPE_DOUBLE)
    {
        restoreCrossBytesDouble (delta, (uint64_t *)pData, nCols, nRows);
    }
    else
    {
        assert (0);
    }

}

