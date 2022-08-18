#-------------------------------------------------------------------------------
#   Copyright 2016 - 2022 Esri
#
#   Licensed under the Apache License, Version 2.0 (the "License");
#   you may not use this file except in compliance with the License.
#   You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
#   Unless required by applicable law or agreed to in writing, software
#   distributed under the License is distributed on an "AS IS" BASIS,
#   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#   See the License for the specific language governing permissions and
#   limitations under the License.
#
#   A copy of the license and additional notices are located with the
#   source distribution at:
#
#   http://github.com/Esri/lerc/
#
#   Contributors:  Thomas Maurer
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#
#   How to run the test:
#
#   You need 2 files, this file "_lerc.py" and the Lerc dll (for Windows) or the
#   Lerc .so file (for Linux) next to it. Then
#
#   >>> import _lerc
#   >>> _lerc.test()
#
#-------------------------------------------------------------------------------

#-------------------------------------------------------------------------------
#
#   What's new in Lerc 4.0
#
#   An encoded image tile (2D, 3D, or 4D), of data type byte to double, can have
#   any value set to invalid. There are new and hopefully easy to use numpy
#   functions to decode from or encode to Lerc. They make use of the numpy
#   masked arrays.
#
#   (result, npmaArr, nDepth, npmaNoData) = decode_ma(lercBlob)
#
#   _ lercBlob is the compressed Lerc blob as a string buffer or byte array as
#     read in from disk or passed in memory.
#   _ result is 0 for success or an error code for failure.
#   _ npmaArr is the masked numpy array with the data and mask of the same shape.
#   _ nDepth == nValuesPerPixel. E.g., 3 for RGB, or 2 for complex numbers.
#   _ npmaNoData is a 1D masked array of size nBands. It can hold one noData
#     value per band. The caller can usually ignore it as npmaArr has all mask
#     info. It may be useful if the data needs to be Lerc encoded again.
#
#   (result, nBytesWritten, lercBlob) = encode_ma(npmaArr, nDepth, maxZErr,
#                                                 nBytesHint, npmaNoData = None)
#
#   _ npmaArr is the image tile (2D, 3D, or 4D) to be encoded, as a numpy masked
#     array.
#   _ nDepth == nValuesPerPixel. E.g., 3 for RGB, or 2 for complex numbers.
#   _ maxZErr is the max encoding error allowed per value. 0 means lossless.
#   _ nBytesHint can be
#     _ 0 - compute num bytes needed for output buffer, but do not encode it (faster than encode)
#     _ 1 - do both, compute exact buffer size needed and encode (slower than encode alone)
#     _ > 1 - create buffer of that given size and encode, if buffer too small encode will fail.
#   _ npmaNoData is a 1D masked array of size nBands. It can hold one noData
#     value per band. It can be used as an alternative to masks. It must be
#     used for the so called mixed case of valid and invalid values at the same
#     pixel, only possible for nDepth > 1. In most cases None can be passed.
#     Note Lerc does not take NaN as a noData value here. It is enough to set the
#     data values to NaN and not specify a noData value.
#
#   As an alternative to the numpy masked array above, there is also the option
#   to have data and masked as separate numpy arrays.
#
#   (result, npArr, npValidMask, npmaNoData) = decode_4D(lercBlob)
#
#   Here, npArr can be of the same shapes as npmaArr above, but it is a regular
#   numpy array, not a masked array. The mask is passed separately as a regular
#   numpy array of type bool. Note that in contrast to the masked array above,
#   True means now valid and False means invalid. The npValidMask can have the
#   following shapes:
#   _ None, all pixels are valid or are marked invalid using noData value or NaN
#   _ 2D or (nRows, nCols), same mask for all bands.
#   _ 3D or (nBands, nRows, nCols), one mask per band.

#   The _4D() functions may work well if all pixels are valid, or nDepth == 1,
#   and the shape of the mask here matches the shape of the data anyway.
#   In such cases the use of a numpy masked array might not be needed or
#   considered an overkill.
#
#   Similar for encode:
#
#   (result, nBytesWritten, lercBlob) = encode_4D(npArr, nDepth, npValidMask,
#                                                 maxZErr, nBytesHint, npmaNoData = None)
#
#   Note that for all encode functions, you can set values to invalid using a
#   mask, or using a noData value, or using NaN (for data types float or double).
#   Or any combination which is then merged using AND for valid (same as OR for invalid)
#   by the Lerc API.
#
#   The decode functions, however, return this info as a mask, wherever possible.
#   Only for nDepth > 1 and the mixed case of valid and invalid values at the same
#   pixel, a noData value is used internally. NaN is never returned by decode.
#
#   The existing Lerc 3.0 encode and decode functions can still be used.
#   Only for nDepth > 1 the mixed case cannot be encoded. If the decoder
#   should encounter a Lerc blob with such a mixed case, it will fail with
#   the error code LercNS::ErrCode::HasNoData == 5.
#
#-------------------------------------------------------------------------------

import numpy as np
import ctypes as ct
from timeit import default_timer as timer
import platform
import os

def _get_lib():
    dir_path = os.path.dirname(os.path.realpath(__file__))

    if platform.system() == "Windows":
        lib = os.path.join(dir_path, 'Lerc.dll')
    elif platform.system() == "Linux":
        lib = os.path.join(dir_path, 'libLerc.so.4')
    elif platform.system() == "Darwin":
        lib = os.path.join(dir_path, 'libLerc.dylib')
    else:
        lib = None

    if not lib or not os.path.exists(lib):
        import ctypes.util
        lib = ctypes.util.find_library('Lerc')

    return lib

lercDll = ct.CDLL (_get_lib())
del _get_lib

#-------------------------------------------------------------------------------

# helper functions:

# data types supported by Lerc, all little endian byte order

def getLercDatatype(npDtype):
    switcher = {
        np.dtype('b'): 0,  # char   or int8
        np.dtype('B'): 1,  # byte   or uint8
        np.dtype('h'): 2,  # short  or int16
        np.dtype('H'): 3,  # ushort or uint16
        np.dtype('i'): 4,  # int    or int32
        np.dtype('I'): 5,  # uint   or uint32
        np.dtype('f'): 6,  # float  or float32
        np.dtype('d'): 7   # double or float64
        }
    return switcher.get(npDtype, -1)

#-------------------------------------------------------------------------------

# Lerc expects an image of size nRows x nCols.
# Optional, it allows multiple values per pixel, like [RGB, RGB, RGB, ... ].
# Or an array of values per pixel. As a 3rd dimension.
# Optional, it allows multiple bands. As an outer 3rd or 4th dimension.

def getLercShape(npArr, nValuesPerPixel):
    nBands = 1
    dim = npArr.ndim
    npShape = npArr.shape

    if nValuesPerPixel == 1:
        if dim == 2:
            (nRows, nCols) = npShape
        elif dim == 3:
            (nBands, nRows, nCols) = npShape  # or band interleaved
    elif nValuesPerPixel > 1:
        if dim == 3:
            (nRows, nCols, nValpp) = npShape  # or pixel interleaved
        elif dim == 4:
            (nBands, nRows, nCols, nValpp) = npShape  # 4D array
        if nValpp != nValuesPerPixel:
            return (0, 0, 0)

    return (nBands, nRows, nCols)

#-------------------------------------------------------------------------------

# Lerc version 3.0
# use only if all pixel values are valid.

def findMaxZError(npArr1, npArr2):
    npDiff = npArr2 - npArr1
    yMin = np.amin(npDiff)
    yMax = np.amax(npDiff)
    return max(abs(yMin), abs(yMax))

# Lerc version 4.0
# honors byte masks and works with noData value: if decoded output
# has a noData value, the orig input must have the same.

def findMaxZError_4D(npDataOrig, npDataDec, npValidMaskDec, nBands):
    
    npDiff = npDataDec - npDataOrig
    
    if (npValidMaskDec is None):
        zMin = np.amin(npDiff)
        zMax = np.amax(npDiff)
    else:
        if not npValidMaskDec.any():    # if all pixel values are void
            return 0
        
        if nBands == 1 or npValidMaskDec.ndim == 3:  # one mask per band
            zMin = np.amin(npDiff[npValidMaskDec])
            zMax = np.amax(npDiff[npValidMaskDec])
        elif nBands > 1:  # same mask for all bands
            zMin = float("inf")
            zMax = -zMin
            for m in range(nBands):
                zMin = min(np.amin(npDiff[m][npValidMaskDec]), zMin)
                zMax = max(np.amax(npDiff[m][npValidMaskDec]), zMax)

    return max(abs(zMin), abs(zMax))

# Lerc version 4.0, using masked arrays

def findMaxZError_ma(npmaArrOrig, npmaArrDec):
    npDiff = npmaArrDec - npmaArrOrig
    yMin = np.amin(npDiff)
    yMax = np.amax(npDiff)
    return max(abs(yMin), abs(yMax))

#-------------------------------------------------------------------------------

# Lerc version 3.0

def findDataRange(npArr, bHasMask, npValidMask, nBands, printInfo = False):
    if printInfo:
        start = timer()

    if not bHasMask or npValidMask is None:
        zMin = np.amin(npArr)
        zMax = np.amax(npArr)
    else:
        if not npValidMask.any():    # if all pixel values are void
            return (-1, -1)
        
        if nBands == 1 or npValidMask.ndim == 3:  # one mask per band
            zMin = np.amin(npArr[npValidMask])
            zMax = np.amax(npArr[npValidMask])
        elif nBands > 1:  # same mask for all bands
            zMin = float("inf")
            zMax = -zMin
            for m in range(nBands):
                zMin = min(np.amin(npArr[m][npValidMask]), zMin)
                zMax = max(np.amax(npArr[m][npValidMask]), zMax)

    if printInfo:
        end = timer()
        print('time findDataRange() = ', (end - start))
    return (zMin, zMax)

# Lerc version 4.0, using masked array

def findDataRange_ma(npmaArr):
    if not npmaArr.any():
        return (-1, -1)
    zMin = np.amin(npmaArr)
    zMax = np.amax(npmaArr)
    return (zMin, zMax)

#-------------------------------------------------------------------------------

# see include/Lerc_c_api.h

lercDll.lerc_computeCompressedSize.restype = ct.c_uint
lercDll.lerc_computeCompressedSize.argtypes = (ct.c_void_p, ct.c_uint, ct.c_int, ct.c_int, ct.c_int,
                                               ct.c_int, ct.c_int, ct.c_char_p, ct.c_double, ct.POINTER(ct.c_uint))

lercDll.lerc_encode.restype = ct.c_uint
lercDll.lerc_encode.argtypes = (ct.c_void_p, ct.c_uint, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int,
                                ct.c_char_p, ct.c_double, ct.c_char_p, ct.c_uint, ct.POINTER(ct.c_uint))

lercDll.lerc_getBlobInfo.restype = ct.c_uint
lercDll.lerc_getBlobInfo.argtypes = (ct.c_char_p, ct.c_uint, ct.POINTER(ct.c_uint),
                                     ct.POINTER(ct.c_double), ct.c_int, ct.c_int)

lercDll.lerc_getDataRanges.restype = ct.c_uint
lercDll.lerc_getDataRanges.argtypes = (ct.c_char_p, ct.c_uint, ct.c_int, ct.c_int,
                                       ct.POINTER(ct.c_double), ct.POINTER(ct.c_double))

lercDll.lerc_decode.restype = ct.c_uint
lercDll.lerc_decode.argtypes = (ct.c_char_p, ct.c_uint, ct.c_int, ct.c_char_p, ct.c_int,
                                ct.c_int, ct.c_int, ct.c_int, ct.c_uint, ct.c_void_p)

# the new _4D functions

lercDll.lerc_computeCompressedSize_4D.restype = ct.c_uint
lercDll.lerc_computeCompressedSize_4D.argtypes = (ct.c_void_p, ct.c_uint, ct.c_int, ct.c_int, ct.c_int,
                                                  ct.c_int, ct.c_int, ct.c_char_p, ct.c_double,
                                                  ct.POINTER(ct.c_uint), ct.c_char_p, ct.POINTER(ct.c_double))

lercDll.lerc_encode_4D.restype = ct.c_uint
lercDll.lerc_encode_4D.argtypes = (ct.c_void_p, ct.c_uint, ct.c_int, ct.c_int, ct.c_int, ct.c_int,
                                   ct.c_int, ct.c_char_p, ct.c_double, ct.c_char_p, ct.c_uint,
                                   ct.POINTER(ct.c_uint), ct.c_char_p, ct.POINTER(ct.c_double))

lercDll.lerc_decode_4D.restype = ct.c_uint
lercDll.lerc_decode_4D.argtypes = (ct.c_char_p, ct.c_uint, ct.c_int, ct.c_char_p, ct.c_int,
                                   ct.c_int, ct.c_int, ct.c_int, ct.c_uint,
                                   ct.c_void_p, ct.c_char_p, ct.POINTER(ct.c_double))

#-------------------------------------------------------------------------------

# npArr can be 2D, 3D, or 4D array. See also getLercShape() above.
#
# npValidMask can be None (bHasMask == False), 2D byte array, or 3D byte array (bHasMask == True).
# if 2D or [nRows, nCols], it is one mask for all bands. 1 means pixel is valid, 0 means invalid.
# if 3D or [nBands, nRows, nCols], it is one mask PER band.
#
# nBytesHint can be
#  0 - compute num bytes needed for output buffer, but do not encode it (faster than encode)
#  1 - do both, compute exact buffer size needed and encode (slower than encode alone)
#  > 1 - create buffer of that given size and encode, if buffer too small encode will fail.
#
# Lerc version 4.0 can also support the mixed case of valid and invalid values at the same pixel.
# As this case is rather special, instead of changing the valid / invalid byte mask representation,
# the concept of a noData value is used.
# For this, pass a 1D masked array of size nBands and data type double. For each band that uses a noData value,
# set the mask value to False, not masked, and the double value to the noData value used.
# Note that Lerc will push the noData value to the valid / invalid byte mask wherever possible.
# On Decode, Lerc only returns the noData values used for those bands where the byte mask cannot represent
# the void values.

def encode(npArr, nValuesPerPixel, bHasMask, npValidMask, maxZErr, nBytesHint, printInfo = False):
    return _encode_Ext(npArr, nValuesPerPixel, npValidMask, maxZErr, nBytesHint, None, printInfo)

def encode_4D(npArr, nValuesPerPixel, npValidMask, maxZErr, nBytesHint, npmaNoDataPerBand = None, printInfo = False):
    return _encode_Ext(npArr, nValuesPerPixel, npValidMask, maxZErr, nBytesHint, npmaNoDataPerBand, printInfo)
    
def _encode_Ext(npArr, nValuesPerPixel, npValidMask, maxZErr, nBytesHint, npmaNoData, printInfo):
    global lercDll

    fctErr = 'Error in _encode_Ext(): '
    
    dataType = getLercDatatype(npArr.dtype)
    if dataType == -1:
        print(fctErr, 'unsupported numpy data type.')
        return (-1, 0)

    (nBands, nRows, nCols) = getLercShape(npArr, nValuesPerPixel)
    if nBands == 0:
        print(fctErr, 'unsupported numpy array shape.')
        return (-1, 0)

    nMasks = 0
    if npValidMask is not None:
        (nMasks, nRows2, nCols2) = getLercShape(npValidMask, 1)
        if not(nMasks == 0 or nMasks == 1 or nMasks == nBands) or not(nRows2 == nRows and nCols2 == nCols):
            print(fctErr, 'unsupported mask array shape.')
            return (-1, 0)

    if (npmaNoData is not None):
        if (len(npmaNoData) != nBands):
            print(fctErr, 'noData array must be of size nBands or None.')
            return (-1, 0)

        noDataArr = np.zeros([nBands], 'd')
        npHasNoData = np.zeros([nBands], 'B')

        for m in range(nBands):
            if not npmaNoData.mask[m]:
                noDataArr[m] = npmaNoData[m]
                npHasNoData[m] = 1

        hasNoData = npHasNoData.tobytes('C')
        cpHasNoData = ct.cast(hasNoData, ct.c_char_p)

        tempArr = noDataArr.tobytes('C')
        cpNoData = ct.cast(tempArr, ct.POINTER(ct.c_double))

    else:
        cpHasNoData = None
        cpNoData = None
        
    if printInfo:
        print('dataType = ', dataType)
        print('nBands = ', nBands)
        print('nRows = ', nRows)
        print('nCols = ', nCols)
        print('nValuesPerPixel = ', nValuesPerPixel)
        print('nMasks = ', nMasks)
        if cpHasNoData:
            print('has noData value')
    
    byteArr = npArr.tobytes('C')  # C order
    cpData = ct.cast(byteArr, ct.c_void_p)

    if npValidMask is not None:
        npValidBytes = npValidMask.astype('B')
        validArr = npValidBytes.tobytes('C')
        cpValidArr = ct.cast(validArr, ct.c_char_p)
    else:
        cpValidArr = None

    ptr = ct.cast((ct.c_uint * 1)(), ct.POINTER(ct.c_uint))
    
    if nBytesHint == 0 or nBytesHint == 1:
        if printInfo:
            start = timer()

        result = lercDll.lerc_computeCompressedSize_4D(cpData, dataType, nValuesPerPixel, nCols, nRows, nBands,
                                                       nMasks, cpValidArr, maxZErr, ptr, cpHasNoData, cpNoData)
        nBytesNeeded = ptr[0]

        if result > 0:
            print(fctErr, 'lercDll.lerc_computeCompressedSize_4D() failed with error code = ', result)
            return (result, 0)

        if printInfo:
            end = timer()
            print('time lerc_computeCompressedSize_4D() = ', (end - start))
    else:
        nBytesNeeded = nBytesHint

    if nBytesHint > 0:
        outBytes = ct.create_string_buffer(nBytesNeeded)
        cpOutBuffer = ct.cast(outBytes, ct.c_char_p)

        if printInfo:
            start = timer()

        result = lercDll.lerc_encode_4D(cpData, dataType, nValuesPerPixel, nCols, nRows, nBands, nMasks, cpValidArr,
                                        maxZErr, cpOutBuffer, nBytesNeeded, ptr, cpHasNoData, cpNoData)
        nBytesWritten = ptr[0]

        if result > 0:
            print(fctErr, 'lercDll.lerc_encode_4D() failed with error code = ', result)
            return (result, 0)

        if printInfo:
            end = timer()
            print('time lerc_encode_4D() = ', (end - start))

    if nBytesHint == 0:
        return (result, nBytesNeeded)
    else:
        return (result, nBytesWritten, outBytes)

#-------------------------------------------------------------------------------

def _has_mixed_case(uv, nValuesPerPixel, iBand):
    fctErr = 'In function _has_mixed_case(): '
    if ((uv.len == 1 and not (uv[0] == 0 or uv[0] == nValuesPerPixel)) or
        (uv.len == 2 and not (uv[0] == 0 and uv[1] == nValuesPerPixel)) or
        (uv.len > 2)):
        print(fctErr, 'mixed case detected of valid and invalid values at the same pixel for band ',
              iBand, ', please provide a noData value')
        return True
    return False

# encode a masked array;
# for nValuesPerPixel > 1 and a mixed case of some values valid and others invalid at the same pixel,
# caller must provide a noData value for the bands with such a mixed case;

def encode_ma(npmaArr, nValuesPerPixel, maxZErr, nBytesHint, npmaNoDataPerBand = None, printInfo = False):

    fctErr = 'Error in encode_ma(): '

    if nValuesPerPixel == 1:
        return _encode_Ext(npmaArr.data, nValuesPerPixel, np.logical_not(npmaArr.mask),
                           maxZErr, nBytesHint, npmaNoDataPerBand, printInfo)

    elif nValuesPerPixel > 1:

        npArr = npmaArr.data

        if npmaNoDataPerBand is not None:
            # for each band that has noData value, fill all masked values with that noData value
            if npmaArr.ndim == 3:    # nBands == 1
                if not npmaNoDataPerBand.mask[0]:
                    npArr = np.ma.filled(npmaArr, npmaNoDataPerBand[0])
                    return _encode_Ext(npArr, nValuesPerPixel, None, maxZErr, nBytesHint, npmaNoDataPerBand, printInfo)

            elif npmaArr.ndim == 4:    # nBands > 1
                nBands = npmaNoDataPerBand.size
                for m in range(nBands):
                    if not npmaNoDataPerBand.mask[m]:
                        npArr[m] = np.ma.filled(npmaArr[m], npmaNoDataPerBand[m])
                if not np.any(npmaNoDataPerBand.mask):
                    return _encode_Ext(npArr, nValuesPerPixel, None, maxZErr, nBytesHint, npmaNoDataPerBand, printInfo)

        # now we have at least one band w/o a noData value, so we must convert the mask and check there is no mixed case

        # compute sum of most inner dimension,
        # so that resulting array has one dim less, and values are in [0, nDepth]

        intMask = np.sum(npmaArr.mask, axis = npmaArr.mask.ndim - 1, dtype = int)

        # for each band without a noData value, check there is no other value but 0 or nDepth
        # (ensure there is no mixed case)

        if intMask.ndim == 2:    # nBands == 1
            if npmaNoDataPerBand.mask[0]:
                uv = np.unique(intMask)
                if _has_mixed_case(uv, nValuesPerPixel, 0):
                    return (-1, 0)

        elif intMask.ndim == 3:    # nBands > 1
            for m in range(nBands):
                if npmaNoDataPerBand.mask[m]:
                    uv = np.unique(intMask[m])
                    if _has_mixed_case(uv, nValuesPerPixel, m):
                        return (-1, 0)

        # convert this int mask back to boolean
        boolMask = intMask.astype(bool)

        return _encode_Ext(npArr, nValuesPerPixel, np.logical_not(boolMask),
                           maxZErr, nBytesHint, npmaNoDataPerBand, printInfo)

#-------------------------------------------------------------------------------

def getLercBlobInfo(lercBlob, printInfo = False):
    return _getLercBlobInfo_Ext(lercBlob, 0, printInfo)

def getLercBlobInfo_4D(lercBlob, printInfo = False):
    return _getLercBlobInfo_Ext(lercBlob, 1, printInfo)

def _getLercBlobInfo_Ext(lercBlob, nSupportNoData, printInfo):
    global lercDll

    fctErr = 'Error in _getLercBlobInfo_Ext(): '
    
    info = ['codec version', 'data type', 'nValuesPerPixel', 'nCols', 'nRows', 'nBands', 'nValidPixels',
            'blob size', 'nMasks', 'nDepth', 'nUsesNoDataValue']
    
    dataRange = ['zMin', 'zMax', 'maxZErrorUsed']

    nBytes = len(lercBlob)
    len0 = len(info)
    len1 = len(dataRange)
    p0 = ct.cast((ct.c_uint * len0)(), ct.POINTER(ct.c_uint))
    p1 = ct.cast((ct.c_double * len1)(), ct.POINTER(ct.c_double))
    cpBytes = ct.cast(lercBlob, ct.c_char_p)
    
    result = lercDll.lerc_getBlobInfo(cpBytes, nBytes, p0, p1, len0, len1)
    if result > 0:
        print(fctErr, 'lercDLL.lerc_getBlobInfo() failed with error code = ', result)
        if nSupportNoData:
            return (result, 0,0,0,0,0,0,0,0,0,0,0,0,0)
        else:
            return (result, 0,0,0,0,0,0,0,0,0,0,0,0)
            
    if printInfo:
        for i in range(len0):
            print(info[i], p0[i])
        for i in range(len1):
            print(dataRange[i], p1[i])

    nUsesNoDataValue = p0[10]    # new key 'nUsesNoDataValue'

    if nUsesNoDataValue and not nSupportNoData:
        print(fctErr, 'This Lerc blob uses noData value. Please upgrade to \
              Lerc version 4.0 functions or newer that support this.')
        return (5, 0,0,0,0,0,0,0,0,0,0,0,0)    # 5 == LercNS::ErrCode::HasNoData
        
    if not nSupportNoData:    # old version, up to Lerc version 3.0
        return (result,
            p0[0], p0[1], p0[2], p0[3], p0[4], p0[5], p0[6], p0[7], p0[8],
            p1[0], p1[1], p1[2])
    else:    # newer version, >= Lerc version 4.0
        return (result,
            p0[0], p0[1], p0[2], p0[3], p0[4], p0[5], p0[6], p0[7], p0[8],
            p1[0], p1[1], p1[2],
            nUsesNoDataValue)    # append to the end

#-------------------------------------------------------------------------------

# This still works same as before for Lerc blobs encoded using data and valid / invalid byte masks,
# but it will fail if the Lerc blob contains noData value (for mixed case of
# valid and invalid values at the same pixel).
# To be fixed in a future release (needs a codec upgrade). 

def getLercDataRanges(lercBlob, nDepth, nBands, printInfo = False):
    global lercDll

    nBytes = len(lercBlob)
    len0 = nDepth * nBands;

    cpBytes = ct.cast(lercBlob, ct.c_char_p)

    mins = ct.create_string_buffer(len0 * 8)
    maxs = ct.create_string_buffer(len0 * 8)
    cpMins = ct.cast(mins, ct.POINTER(ct.c_double))
    cpMaxs = ct.cast(maxs, ct.POINTER(ct.c_double))

    if printInfo:
        start = timer()

    result = lercDll.lerc_getDataRanges(cpBytes, nBytes, nDepth, nBands, cpMins, cpMaxs)

    if result > 0:
        print('Error in getLercDataRanges(): lercDLL.lerc_getDataRanges() failed with error code = ', result)
        return (result)

    if printInfo:
        end = timer()
        print('time lerc_getDataRanges() = ', (end - start))
        print('data ranges per band and depth:')
        for i in range(nBands):
            for j in range(nDepth):
                print('band', i, 'depth', j, ': [', cpMins[i * nDepth + j], ',', cpMaxs[i * nDepth + j], ']')

    npMins = np.frombuffer(mins, 'd')
    npMaxs = np.frombuffer(maxs, 'd')
    npMins.shape = (nBands, nDepth)
    npMaxs.shape = (nBands, nDepth)

    return (result, npMins, npMaxs)

#-------------------------------------------------------------------------------

def decode(lercBlob, printInfo = False):
    return _decode_Ext(lercBlob, 0, printInfo)

def decode_4D(lercBlob, printInfo = False):
    return _decode_Ext(lercBlob, 1, printInfo)

def _decode_Ext(lercBlob, nSupportNoData, printInfo):
    global lercDll

    fctErr = 'Error in _decode_Ext(): '

    (result, version, dataType, nValuesPerPixel, nCols, nRows, nBands, nValidPixels, blobSize,
     nMasks, zMin, zMax, maxZErrUsed, nUsesNoData) = getLercBlobInfo_4D(lercBlob, printInfo)
    if result > 0:
        print(fctErr, 'getLercBlobInfo() failed with error code = ', result)
        return result

    if nUsesNoData and not nSupportNoData:
        print(fctErr, 'This Lerc blob uses noData value. Please upgrade to \
              Lerc version 4.0 functions or newer that support this.')
        return (5, None, None)    # 5 == LercNS::ErrCode::HasNoData

    # convert Lerc dataType to np data type
    npDtArr = ['b', 'B', 'h', 'H', 'i', 'I', 'f', 'd']
    npDtype = npDtArr[dataType]

    # convert Lerc shape to np shape
    if nBands == 1:
        if nValuesPerPixel == 1:
            shape = (nRows, nCols)
        elif nValuesPerPixel > 1:
            shape = (nRows, nCols, nValuesPerPixel)
    elif nBands > 1:
        if nValuesPerPixel == 1:
            shape = (nBands, nRows, nCols)
        elif nValuesPerPixel > 1:
            shape = (nBands, nRows, nCols, nValuesPerPixel)

    # create empty buffer for decoded data
    dataSize = [1, 1, 2, 2, 4, 4, 4, 8]
    nBytes = nBands * nRows * nCols * nValuesPerPixel * dataSize[dataType]
    dataBuf = ct.create_string_buffer(nBytes)
    cpData = ct.cast(dataBuf, ct.c_void_p)
    cpBytes = ct.cast(lercBlob, ct.c_char_p)

    # create empty buffer for valid pixels masks, if needed
    cpValidArr = None
    if nMasks > 0:
        validBuf = ct.create_string_buffer(nMasks * nRows * nCols)
        cpValidArr = ct.cast(validBuf, ct.c_char_p)

    # create empty buffer for noData arrays, if needed
    cpHasNoDataArr = None
    cpNoDataArr = None
    if (nUsesNoData):
        hasNoDataBuf = ct.create_string_buffer(nBands)
        noDataBuf = ct.create_string_buffer(nBands * 8)
        cpHasNoDataArr = ct.cast(hasNoDataBuf, ct.c_char_p)
        cpNoDataArr = ct.cast(noDataBuf, ct.POINTER(ct.c_double))

    # call decode
    if printInfo:
        start = timer()

    result = lercDll.lerc_decode_4D(cpBytes, len(lercBlob), nMasks, cpValidArr, nValuesPerPixel,
                                    nCols, nRows, nBands, dataType, cpData, cpHasNoDataArr, cpNoDataArr)

    if result > 0:
        print(fctErr, 'lercDll.lerc_decode() failed with error code = ', result)
        return result

    if printInfo:
        end = timer()
        print('time lerc_decode() = ', (end - start))

    # return result, np data array, and np valid pixels array if there
    npArr = np.frombuffer(dataBuf, npDtype)
    npArr.shape = shape

    npValidMask = None
    if nMasks > 0:
        npValidBytes = np.frombuffer(validBuf, dtype='B')
        if nMasks == 1:
            npValidBytes.shape = (nRows, nCols)
        else:
            npValidBytes.shape = (nMasks, nRows, nCols)
        npValidMask = (npValidBytes != 0)

    npmaNoData = None
    if (nUsesNoData):
        npHasNoData = np.frombuffer(hasNoDataBuf, dtype='B')
        npHasNoData.shape = (nBands)
        npHasNoDataMask = (npHasNoData == 0)
        npNoData = np.frombuffer(noDataBuf, 'd')
        npNoData.shape = (nBands)
        npmaNoData = np.ma.array(npNoData, mask = npHasNoDataMask)

    if not nSupportNoData:    # old version, up to Lerc version 3.0
        return (result, npArr, npValidMask)
    else:    # newer version, >= Lerc version 4.0
        return (result, npArr, npValidMask, npmaNoData)

#-------------------------------------------------------------------------------

# return data as a masked array; convenient but slower

def decode_ma(lercBlob, printInfo = False):

    fctErr = 'Error in decode_ma(): '

    (result, version, dataType, nValuesPerPixel, nCols, nRows, nBands, nValidPixels,
     blobSize, nMasks, zMin, zMax, maxZErrUsed, nUsesNoData) = getLercBlobInfo_4D(lercBlob, printInfo)
    if result > 0:
        print(fctErr, 'getLercBlobInfo() failed with error code = ', result)
        return result

    (result, npArr, npValidMask, npmaNoData) = _decode_Ext(lercBlob, 1, printInfo)
    if result > 0:
        print(fctErr, '_decode_Ext() failed with error code = ', result)
        return result

    npmaArr = convert2ma(npArr, npValidMask, nValuesPerPixel, nBands, npmaNoData)

    return (result, npmaArr, nValuesPerPixel, npmaNoData)

# convert numpy data array, valid / invalid byte mask, and noData values to one masked array

def convert2ma(npArr, npValidMask, nValuesPerPixel, nBands, npmaNoData):

    if (npmaNoData is None) and (npValidMask is None):
        return(np.ma.array(npArr, mask = False))

    if not npValidMask is None:

        if (nValuesPerPixel > 1):    # need to blow up mask from 2D to 3D or 3D to 4D

            npMask3D = npValidMask
            for k in range(nValuesPerPixel - 1):
                npMask3D = np.dstack((npMask3D, npValidMask))

            if nBands == 1 or npValidMask.ndim == 3:  # one mask per band
                npmaArr = np.ma.array(npArr, mask = (npMask3D == False))

            elif nBands > 1:  # use same mask for all bands
                npMask4D = np.stack([npMask3D for _ in range(nBands)])
                npmaArr = np.ma.array(npArr, mask = (npMask4D == False))

        elif (nValuesPerPixel == 1):
            if nBands == 1 or npValidMask.ndim == 3:  # one mask per band
                npmaArr = np.ma.array(npArr, mask = (npValidMask == False))

            elif nBands > 1:  # same mask for all bands
                npMask3D = np.stack([npValidMask for _ in range(nBands)])
                npmaArr = np.ma.array(npArr, mask = (npMask3D == False))

    elif npValidMask is None:
        npmaArr = np.ma.array(npArr, mask = False)

    if not npmaNoData is None:

        if nBands == 1:
            if not npmaNoData.mask[0]:
                npmaArr = np.ma.masked_equal(npmaArr, npmaNoData[0])

        elif nBands > 1:
            for m in range(nBands):
                if not npmaNoData.mask[m]:
                    npmaArr[m] = np.ma.masked_equal(npmaArr[m], npmaNoData[m])

    return (npmaArr)

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------

def test():

    fctErr = 'Error in test(): '

    # data types supported by Lerc, all little endian byte order
    # 'b', 'B', 'h', 'H', 'i', 'I', 'f', 'd'

    print('\n -------- encode test 1 -------- ')
    
    nBands = 1
    nRows = 256
    nCols = 256
    nValuesPerPixel = 3  # values or array per pixel, could be RGB values or hyper spectral image
    
    npArr = np.zeros((nRows, nCols, nValuesPerPixel), 'f', 'C')  # data type float, C order
    #npValidMask = np.full((nRows, nCols), True)  # set all pixels valid
    npValidMask = None  # same as all pixels valid
    maxZErr = 0.001

    # fill it with something
    for i in range(nRows):
        for j in range(nCols):
            for k in range(nValuesPerPixel):
                npArr[i][j][k] = 0.001 * i * j + k

    # call with buffer size 0 to only compute compressed size, optional
    numBytesNeeded = 0
    (result, numBytesNeeded) = encode(npArr, nValuesPerPixel, False, npValidMask,
                                      maxZErr, numBytesNeeded, True)
    if result > 0:
        print(fctErr, 'encode() failed with error code = ', result)
        return result
    print('computed compressed size = ', numBytesNeeded)

    # encode with numBytesNeeded from above or big enough estimate
    (result, numBytesWritten, outBuffer) = encode(npArr, nValuesPerPixel, False, npValidMask,
                                                  maxZErr, numBytesNeeded, True)
    if result > 0:
        print(fctErr, 'encode() failed with error code = ', result)
        return result
    print('num bytes written to buffer = ', numBytesWritten)

    # decode again
    (result, npArrDec, npValidMaskDec) = decode(outBuffer, True)
    if result > 0:
        print(fctErr, 'decode() failed with error code = ', result)
        return result

    # evaluate the difference to orig
    maxZErrFound = findMaxZError_4D(npArr, npArrDec, npValidMaskDec, nBands)
    print('maxZErr found = ', maxZErrFound)

    # find the range [zMin, zMax] in the numpy array
    (zMin, zMax) = findDataRange(npArrDec, False, None, nBands, True)
    print('data range found = ', zMin, zMax)

    print('\n -------- encode test 2 -------- ')

    nBands = 3
    nRows = 256
    nCols = 256
    nValuesPerPixel = 1
    
    npArr = np.zeros((nBands, nRows, nCols), 'f', 'C')  # data type float, C order
    npValidMask = np.full((nRows, nCols), True)  # set all pixels valid
    maxZErr = 0.001

    # fill it with something
    for m in range(nBands):
        for i in range(nRows):
            for j in range(nCols):
                npArr[m][i][j] = 0.001 * i * j + m

    # encode
    nBytesBigEnough = npArr.nbytes * 2
    (result, numBytesWritten, outBuffer) = encode(npArr, nValuesPerPixel, True, npValidMask,
                                                  maxZErr, nBytesBigEnough, True)
    if result > 0:
        print(fctErr, 'encode() failed with error code = ', result)
        return result
    print('num bytes written to buffer = ', numBytesWritten)

    # decode again
    (result, npArrDec, npValidMaskDec) = decode(outBuffer, True)
    if result > 0:
        print(fctErr, 'decode() failed with error code = ', result)
        return result

    # evaluate the difference to orig
    maxZErrFound = findMaxZError_4D(npArr, npArrDec, npValidMaskDec, nBands)
    print('maxZErr found = ', maxZErrFound)

    # find the range [zMin, zMax]
    (zMin, zMax) = findDataRange(npArrDec, True, npValidMaskDec, nBands, True)
    print('data range found = ', zMin, zMax)
    
    # save compressed Lerc blob to disk
    #open('C:/temp/test_1_256_256_3_double.lrc', 'wb').write(outBuffer)

    print('\n -------- encode test 3 -------- ')

    # example for the new _4D() and _ma() functions in Lerc version 4.0
    
    nBands = 3
    nRows = 512
    nCols = 512
    nValuesPerPixel = 2  # values or array per pixel

    npArr = np.zeros((nBands, nRows, nCols, nValuesPerPixel), 'f', 'C')  # data type float, C order
    npValidMask = None  # same as all pixels valid, but we are going to add a noData value
    maxZErr = 0.01
    noDataVal = -9999.0
    cntInvalid = 0

    # fill it with something
    start = timer()
    for m in range(nBands):
        for i in range(nRows):
            for j in range(nCols):
                for k in range(nValuesPerPixel):
                    z = 0.001 * i * j + 5 * m + k
                    # for all values at the same pixel, will get pushed into the byte mask
                    if j == i:
                        z = noDataVal
                        cntInvalid += 1
                    # create mixed case, decoded output will use noData for this one pixel in band 0
                    if (m == 0 and i == 5 and j == 7 and k == 0):
                        z = noDataVal
                        cntInvalid += 1
                    npArr[m][i][j][k] = z
    end = timer()
    print('time fill test array = ', (end - start))

    # prepare noData arrays
    npNoDataArr = np.zeros((nBands), 'd')    # noData value is always type double
    npNoDataArr.fill(noDataVal)    # noData value can vary between bands
    npmaNoData = np.ma.array(npNoDataArr, mask = False)    # input has noData values in all 3 bands

    # part A, using _4D() functions:
    
    # encode using encode_4D()
    start = timer()
    nBytesBigEnough = npArr.nbytes * 2
    (result, numBytesWritten, outBuffer) = encode_4D(npArr, nValuesPerPixel, npValidMask, maxZErr,
                                                     nBytesBigEnough, npmaNoData, False)
    end = timer()
    if result > 0:
        print(fctErr, 'encode_4D() failed with error code = ', result)
        return result
    print('time encode_4D() = ', (end - start))
    print('num bytes written to buffer = ', numBytesWritten)

    # decode using decode_4D()
    start = timer()
    (result, npArrDec, npValidMaskDec, npmaNoDataDec) = decode_4D(outBuffer, False)
    end = timer()
    if result > 0:
        print(fctErr, 'decode_4D() failed with error code = ', result)
        return result
    print('time decode_4D() = ', (end - start))

    # evaluate the difference to orig
    maxZErrFound = findMaxZError_4D(npArr, npArrDec, npValidMaskDec, nBands)
    print('maxZErr found = ', maxZErrFound)

    # find the range [zMin, zMax]
    npmaArrDec = convert2ma(npArrDec, npValidMaskDec, nValuesPerPixel, nBands, npmaNoDataDec)
    (zMin, zMax) = findDataRange_ma(npmaArrDec)
    print('data range found = ', zMin, zMax)

    # part B, using _ma() functions or masked arrays:

    npmaArr = np.ma.array(npArr, mask = False)
    start = timer()
    (result, numBytesWritten2, outBuffer2) = encode_ma(npmaArr, nValuesPerPixel, maxZErr,
                                                       nBytesBigEnough, npmaNoData, False)
    end = timer()
    if result > 0:
        print(fctErr, 'encode_ma() failed with error code = ', result)
        return result
    print('time encode_ma() = ', (end - start))
    print('num bytes written to buffer = ', numBytesWritten2)
    print('lerc blob size from encode_4D() = ', numBytesWritten,
          ', and from encode_ma() = ', numBytesWritten2)

    #decode using decode_ma()
    start = timer()
    (result, npmaArrDec, nDepthDec, npmaNoDataDec2) = decode_ma(outBuffer2, False)
    end = timer()
    if result > 0:
        print(fctErr, 'decode_ma() failed with error code = ', result)
        return result
    print('time decode_ma() = ', (end - start))

    # find the range [zMin, zMax], again
    (zMin, zMax) = findDataRange_ma(npmaArrDec)
    print('data range found for ma = ', zMin, zMax)
    print('number of invalid values, orig = ', cntInvalid, ', in masked array = ',
          np.ma.count_masked(npmaArrDec))


    if False:
        print('\n -------- decode test on ~100 different Lerc blobs -------- ')

        folder = 'D:/GitHub/LercOpenSource_v2.5/testData/'
        listFile = folder + '_list.txt'
        with open(listFile, 'r') as f:
            lines = f.readlines()
            f.close()
                
        skipFirstLine = True
        for line in lines:
            if skipFirstLine:
                skipFirstLine = False
                continue

            fn = folder + line.rstrip()
            bytesRead = open(fn, 'rb').read()

            # read the blob header, optional
            (result, codecVersion, dataType, nValuesPerPixel, nCols, nRows, nBands, nValidPixels,
             blobSize, nMasks, zMin, zMax, maxZErrUsed, nUsesNoData) = getLercBlobInfo_4D(bytesRead, False)
            if result > 0:
                print(fctErr, 'getLercBlobInfo_4D() failed with error code = ', result)
                return result

            # read the data ranges, optional
            if nUsesNoData == 0:
                (result, npMins, npMaxs) = getLercDataRanges(bytesRead, nValuesPerPixel, nBands, False)
                if result > 0:
                    print(fctErr, 'getLercDataRanges() failed with error code = ', result)
                    return result

            # decode
            (result, npmaArr, nDepth, npmaNoData) = decode_ma(bytesRead, False)
            if result > 0:
                print(fctErr, 'decode_ma() failed with error code = ', result)
                return result

            # find the range [zMin, zMax]
            (zMin, zMax) = findDataRange_ma(npmaArr)
            
            print(f'codec {codecVersion:1}, dt {dataType:1}, nDepth {nValuesPerPixel:3}, nCols {nCols:5},',
                  f'nRows {nRows:5}, nBands {nBands:3}, nMasks {nMasks:3}, maxZErr {maxZErrUsed:.6f},',
                  f'nUsesNoData {nUsesNoData:3}, zMin {zMin:9.3f}, zMax {zMax:14.3f},  ', line.rstrip())

    return result
