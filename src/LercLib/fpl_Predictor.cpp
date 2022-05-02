/*
COPYRIGHT 1995-2021 ESRI

TRADE SECRETS: ESRI PROPRIETARY AND CONFIDENTIAL
Unpublished material - all rights reserved under the
Copyright Laws of the United States.

For additional information, contact:
Environmental Systems Research Institute, Inc.
Attn: Contracts Dept
380 New York Street
Redlands, California, USA 92373

email: contracts@esri.com
*/

/* Analytical Raster Compression.
 * Original coding Yuriy Yakimenko
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
