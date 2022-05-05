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

#include "fpl_Predictor.h"

USING_NAMESPACE_LERC

int Predictor::getMaxByteDelta (const PredictorType p)
{
    return MAX_DELTA - Predictor::getIntDelta (p);
}

PredictorType Predictor::getType (const char code)
{
    if (code == 0) return PREDICTOR_NONE;
    if (code == 1) return PREDICTOR_DELTA1;
    if (code == 2) return PREDICTOR_ROWS_COLS;

    return PREDICTOR_UNKNOWN;
}

unsigned char Predictor::getCode (const PredictorType p)
{
    if (p == PREDICTOR_NONE) return 0;
    if (p == PREDICTOR_DELTA1) return 1;
    if (p == PREDICTOR_ROWS_COLS) return 2;

    return -1;
}


int Predictor::getIntDelta (const PredictorType p)
{
    switch (p)
    {
        case PREDICTOR_NONE:
            return 0;
        case PREDICTOR_ROWS_COLS:
            return 2;
        case PREDICTOR_DELTA1:
            return 1;
        default:
            return 0;
    }

    return 0;

}

PredictorType Predictor::fromDeltaAndCross (int delta, bool cross)
{
    if (delta < 0)
        return PREDICTOR_UNKNOWN;

    else if (delta == 0)
        return PREDICTOR_NONE;

    else if (false == cross)
    {
        if (delta == 1)
          return PREDICTOR_DELTA1;
    }
    else
    {
        if (delta == 2)
          return PREDICTOR_ROWS_COLS;
    }

    return PREDICTOR_UNKNOWN;
}
