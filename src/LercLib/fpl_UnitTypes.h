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

#ifndef FPL_UNIT_TYPES_H
#define FPL_UNIT_TYPES_H

#include <cstddef>
#include <stdint.h>
#include "Defines.h"

NAMESPACE_LERC_START

enum UnitType { UNIT_TYPE_UNKNOWN = 0, UNIT_TYPE_BYTE = 1, UNIT_TYPE_SHORT = 2, UNIT_TYPE_LONG = 3, UNIT_TYPE_64_BIT = 4, UNIT_TYPE_FLOAT = 5, UNIT_TYPE_DOUBLE = 6 };

struct UnitTypes
{
    static size_t size (const UnitType type);
    static UnitType type (int iBytes, bool bFloatType);

    static unsigned char unitCode (const UnitType type) ;

    static void setDerivative (const UnitType type, void *pData, const size_t nCount, const int nLevel, const int start_level = 1);
    static void setRowsDerivative (const UnitType type, void *pData, const size_t nCols, const size_t nRows, const int nLevel, int phase = 0);
    static void setCrossDerivative (const UnitType type, void *pData, const size_t nCols, const size_t nRows, const int nLevel, int phase = 0);

    static void setBlockDerivative (const UnitType type, void *pData, const size_t nCols, const size_t nRows, const int nLevel, const int start_level = 1);

    ////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

    // RESTORE:

    static void restoreBlockSequence (const int delta, void *pData, const size_t nCols, const size_t nRows, const UnitType unit_type);
    static void restoreCrossBytes (const int delta, void *pData, const size_t nCols, const size_t nRows, const UnitType unit_type);

    static void doFloatTransform(uint32_t* pData, const size_t iCnt);
    static void undoFloatTransform(uint32_t* pData, const size_t iCnt);
} ;

NAMESPACE_LERC_END
#endif
