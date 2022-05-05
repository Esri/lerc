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

#ifndef FPL_PREDICTOR_H
#define FPL_PREDICTOR_H

#include "Defines.h"

NAMESPACE_LERC_START

#define MAX_DELTA 5 

enum PredictorType { PREDICTOR_UNKNOWN = -1, PREDICTOR_NONE = 0, PREDICTOR_DELTA1 = 1, PREDICTOR_ROWS_COLS = 2 };

struct Predictor
{
    static int getMaxByteDelta (const PredictorType p);

    static PredictorType getType (const char code);

    static unsigned char getCode (const PredictorType p);

    static int getIntDelta (const PredictorType p);

    static PredictorType fromDeltaAndCross (int delta, bool cross);
};

NAMESPACE_LERC_END
#endif
