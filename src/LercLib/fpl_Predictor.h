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
