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
        lib = os.path.join(dir_path, 'Lerc.so')
    elif platform.system() == "Darwin":
        lib = os.path.join(dir_path, 'Lerc.dylib')
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
# Optional, it allows multiple values per pixel, like [RGB, RGB, RGB, ... ]. Or an array of values per pixel. As a 3rd dimension. 
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

# Lerc version 3.1
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

#-------------------------------------------------------------------------------

# Lerc version 3.0

def findDataRange(npArr, bHasMask, npValidMask, nBands, printInfo = False):
    start = timer()

    if not bHasMask:
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

    end = timer()
    if printInfo:
        print('time findDataRange() = ', (end - start))
    return (zMin, zMax)

# Lerc version 3.1

def findDataRange_4D(npArr, npValidMask, nBands, npHasNoDataArr = None, npNoDataArr = None):

    fctErr = 'Error in findDataRange_4D(): '
    
    if ((npHasNoDataArr is not None) or (npNoDataArr is not None)):
        if ((len(npNoDataArr) != nBands) or (len(npNoDataArr) != nBands)):
            print(fctErr, 'noData arrays must be of size nBands or None.')
            return (-1, -1)

        # honor noData value
        if nBands == 1:
            if npHasNoDataArr[0]:
                npArrMasked = np.ma.masked_array(npArr, npArr == npNoDataArr[0])
            else:
                npArrMasked = npArr
            
            return findDataRange_4D(npArrMasked, npValidMask, 1)
        
        else:
            zMin = float("inf")
            zMax = -zMin
            for m in range(nBands):
                if npValidMask is not None and npValidMask.ndim == 3:    # one mask per band
                    mask = npValidMask[m]
                else:
                    mask = npValidMask

                oneHasNoDataArr = np.full((1), npHasNoDataArr[m])
                oneNoDataArr = np.full((1), npNoDataArr[m])
                
                (a, b) = findDataRange_4D(npArr[m], mask, 1, oneHasNoDataArr, oneNoDataArr)
                
                zMin = min(zMin, a)
                zMax = max(zMax, b)

            return (zMin, zMax)
        
    else:    # no noData value, byte masks only, if any
        if npValidMask is None:
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

        return (zMin, zMax)

#-------------------------------------------------------------------------------

# see include/Lerc_c_api.h

lercDll.lerc_computeCompressedSize.restype = ct.c_uint
lercDll.lerc_computeCompressedSize.argtypes = (ct.c_void_p, ct.c_uint, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_char_p, ct.c_double, ct.POINTER(ct.c_uint))

lercDll.lerc_encode.restype = ct.c_uint
lercDll.lerc_encode.argtypes = (ct.c_void_p, ct.c_uint, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_char_p, ct.c_double, ct.c_char_p, ct.c_uint, ct.POINTER(ct.c_uint))

lercDll.lerc_getBlobInfo.restype = ct.c_uint
lercDll.lerc_getBlobInfo.argtypes = (ct.c_char_p, ct.c_uint, ct.POINTER(ct.c_uint), ct.POINTER(ct.c_double), ct.c_int, ct.c_int)

lercDll.lerc_getDataRanges.restype = ct.c_uint
lercDll.lerc_getDataRanges.argtypes = (ct.c_char_p, ct.c_uint, ct.c_int, ct.c_int, ct.POINTER(ct.c_double), ct.POINTER(ct.c_double))

lercDll.lerc_decode.restype = ct.c_uint
lercDll.lerc_decode.argtypes = (ct.c_char_p, ct.c_uint, ct.c_int, ct.c_char_p, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_uint, ct.c_void_p)

# the new _4D functions

lercDll.lerc_computeCompressedSize_4D.restype = ct.c_uint
lercDll.lerc_computeCompressedSize_4D.argtypes = (ct.c_void_p, ct.c_uint, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_char_p, ct.c_double,
                                                  ct.POINTER(ct.c_uint), ct.c_char_p, ct.POINTER(ct.c_double))

lercDll.lerc_encode_4D.restype = ct.c_uint
lercDll.lerc_encode_4D.argtypes = (ct.c_void_p, ct.c_uint, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_char_p, ct.c_double, ct.c_char_p, ct.c_uint,
                                   ct.POINTER(ct.c_uint), ct.c_char_p, ct.POINTER(ct.c_double))

lercDll.lerc_decode_4D.restype = ct.c_uint
lercDll.lerc_decode_4D.argtypes = (ct.c_char_p, ct.c_uint, ct.c_int, ct.c_char_p, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_uint,
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
# Lerc version 3.1 can also support the mixed case of valid and invalid values at the same pixel.
# As this case is rather special, instead of changing the valid / invalid byte mask representation,
# the concept of a noData value is used.
# For this, pass a byte array and a double array, both of size nBands. For each band that uses a noData value,
# set the byte array value to 1, and the double array value to the noData value used.
# Note that Lerc will push the noData value to the valid / invalid byte mask wherever possible.
# On Decode, Lerc only returns the noData values used for those bands where the byte mask cannot represent
# the void values.

def encode(npArr, nValuesPerPixel, bHasMask, npValidMask, maxZErr, nBytesHint, printInfo = False):
    return _encode_Ext(npArr, nValuesPerPixel, npValidMask, maxZErr, nBytesHint, None, None, printInfo)

def encode_4D(npArr, nValuesPerPixel, npValidMask, maxZErr, nBytesHint, bHasNoDataPerBand = None, noDataPerBand = None, printInfo = False):
    return _encode_Ext(npArr, nValuesPerPixel, npValidMask, maxZErr, nBytesHint, bHasNoDataPerBand, noDataPerBand, printInfo)
    
def _encode_Ext(npArr, nValuesPerPixel, npValidMask, maxZErr, nBytesHint, bHasNoDataPerBand, noDataPerBand, printInfo):
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

    if ((bHasNoDataPerBand is not None) or (noDataPerBand is not None)):
        if ((len(bHasNoDataPerBand) != nBands) or (len(noDataPerBand) != nBands)):
            print(fctErr, 'noData arrays must be of size nBands or None.')
            return (-1, 0)
        
        npHasNoData = bHasNoDataPerBand.astype('B')
        hasNoData = npHasNoData.tobytes('C')
        cpHasNoData = ct.cast(hasNoData, ct.c_char_p)

        tempArr = noDataPerBand.tobytes('C')
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
        start = timer()
        result = lercDll.lerc_computeCompressedSize_4D(cpData, dataType, nValuesPerPixel, nCols, nRows, nBands,
                                                       nMasks, cpValidArr, maxZErr, ptr, cpHasNoData, cpNoData)
        nBytesNeeded = ptr[0]
        end = timer()

        if result > 0:
            print(fctErr, 'lercDll.lerc_computeCompressedSize_4D() failed with error code = ', result)
            return (result, 0)

        if printInfo:
            print('time lerc_computeCompressedSize_4D() = ', (end - start))
    else:
        nBytesNeeded = nBytesHint

    if nBytesHint > 0:
        outBytes = ct.create_string_buffer(nBytesNeeded)
        cpOutBuffer = ct.cast(outBytes, ct.c_char_p)
        start = timer()
        result = lercDll.lerc_encode_4D(cpData, dataType, nValuesPerPixel, nCols, nRows, nBands, nMasks, cpValidArr,
                                        maxZErr, cpOutBuffer, nBytesNeeded, ptr, cpHasNoData, cpNoData)
        nBytesWritten = ptr[0]
        end = timer()

        if result > 0:
            print(fctErr, 'lercDll.lerc_encode_4D() failed with error code = ', result)
            return (result, 0)

        if printInfo:
            print('time lerc_encode_4D() = ', (end - start))

    if nBytesHint == 0:
        return (result, nBytesNeeded)
    else:
        return (result, nBytesWritten, outBytes)

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
        print(fctErr, 'This Lerc blob uses noData value. Please upgrade to Lerc version 3.1 functions or newer that support this.')
        return (5, 0,0,0,0,0,0,0,0,0,0,0,0)    # 5 == LercNS::ErrCode::HasNoData
        
    if not nSupportNoData:    # old version, up to Lerc version 3.0
        return (result,
            p0[0], p0[1], p0[2], p0[3], p0[4], p0[5], p0[6], p0[7], p0[8],
            p1[0], p1[1], p1[2])
    else:    # newer version, >= Lerc version 3.1
        return (result,
            p0[0], p0[1], p0[2], p0[3], p0[4], p0[5], p0[6], p0[7], p0[8],
            p1[0], p1[1], p1[2],
            nUsesNoDataValue)    # append to the end

#-------------------------------------------------------------------------------

# This still works same as before for Lerc blobs encoded using data and valid / invalid byte masks,
# but it will fail if the Lerc blob contains noData value (for mixed case of valid and invalid values at the same pixel).
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

    start = timer()
    result = lercDll.lerc_getDataRanges(cpBytes, nBytes, nDepth, nBands, cpMins, cpMaxs)
    end = timer()

    if result > 0:
        print('Error in getLercDataRanges(): lercDLL.lerc_getDataRanges() failed with error code = ', result)
        return (result)

    if printInfo:
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

    (result, version, dataType, nValuesPerPixel, nCols, nRows, nBands, nValidPixels, blobSize, nMasks, zMin, zMax, maxZErrUsed, nUsesNoData) = getLercBlobInfo_4D(lercBlob, printInfo)
    if result > 0:
        print(fctErr, 'getLercBlobInfo() failed with error code = ', result)
        return result

    if nUsesNoData and not nSupportNoData:
        print(fctErr, 'This Lerc blob uses noData value. Please upgrade to Lerc version 3.1 functions or newer that support this.')
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
    start = timer()
    result = lercDll.lerc_decode_4D(cpBytes, len(lercBlob), nMasks, cpValidArr, nValuesPerPixel, nCols, nRows, nBands, dataType, cpData, cpHasNoDataArr, cpNoDataArr)
    end = timer()

    if result > 0:
        print(fctErr, 'lercDll.lerc_decode() failed with error code = ', result)
        return result
    
    if printInfo:
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

    npHasNoDataMask = None
    npNoData = None
    if (nUsesNoData):
        npHasNoData = np.frombuffer(hasNoDataBuf, dtype='B')
        npHasNoData.shape = (nBands)
        npHasNoDataMask = (npHasNoData != 0)
        npNoData = np.frombuffer(noDataBuf, 'd')
        npNoData.shape = (nBands)

    if not nSupportNoData:    # old version, up to Lerc version 3.0
        return (result, npArr, npValidMask)
    else:    # newer version, >= Lerc version 3.1
        return (result, npArr, npValidMask, npHasNoDataMask, npNoData)

#-------------------------------------------------------------------------------
#-------------------------------------------------------------------------------

def test():

    # data types supported by Lerc, all little endian byte order
    # 'b', 'B', 'h', 'H', 'i', 'I', 'f', 'd'

    print(' -------- encode test 1 -------- ')
    
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
    (result, numBytesNeeded) = encode(npArr, nValuesPerPixel, False, npValidMask, maxZErr, numBytesNeeded, True)
    if result > 0:
        print('Error in test(): error code = ', result)
        return result
    print('computed compressed size = ', numBytesNeeded)

    # encode with numBytesNeeded from above or big enough estimate
    (result, numBytesWritten, outBuffer) = encode(npArr, nValuesPerPixel, False, npValidMask, maxZErr, numBytesNeeded, True)
    if result > 0:
        print('Error in test(): error code = ', result)
        return result
    print('num bytes written to buffer = ', numBytesWritten)

    # decode again
    (result, npArrDec, npValidMaskDec) = decode(outBuffer, True)
    if result > 0:
        print('Error in test(): decode() failed with error code = ', result)
        return result

    # evaluate the difference to orig
    maxZErrFound = findMaxZError_4D(npArr, npArrDec, npValidMaskDec, nBands)
    print('maxZErr found = ', maxZErrFound)

    # find the range [zMin, zMax] in the numpy array
    #(zMin, zMax) = findDataRange(npArrDec, False, None, nBands, True)
    (zMin, zMax) = findDataRange_4D(npArrDec, None, nBands)
    print('data range found = ', zMin, zMax)

    print(' -------- encode test 2 -------- ')

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
    (result, numBytesWritten, outBuffer) = encode(npArr, nValuesPerPixel, True, npValidMask, maxZErr, nBytesBigEnough, True)
    if result > 0:
        print('Error in encode(): error code = ', result)
        return result
    print('num bytes written to buffer = ', numBytesWritten)

    # decode again
    (result, npArrDec, npValidMaskDec) = decode(outBuffer, True)
    if result > 0:
        print('Error in test(): decode() failed with error code = ', result)
        return result

    # evaluate the difference to orig
    maxZErrFound = findMaxZError_4D(npArr, npArrDec, npValidMaskDec, nBands)
    print('maxZErr found = ', maxZErrFound)

    # find the range [zMin, zMax]
    (zMin, zMax) = findDataRange_4D(npArrDec, npValidMaskDec, nBands)
    print('data range found = ', zMin, zMax)
    
    # save compressed Lerc blob to disk
    #open('C:/temp/test_1_256_256_3_double.lrc', 'wb').write(outBuffer)

    print(' -------- encode test 3 -------- ')

    # example for the new _4D() functions in Lerc version 3.1
    
    nBands = 3
    nRows = 128
    nCols = 128
    nValuesPerPixel = 2  # values or array per pixel

    npArr = np.zeros((nBands, nRows, nCols, nValuesPerPixel), 'f', 'C')  # data type float, C order
    npValidMask = None  # same as all pixels valid, but we are going to add a noData value
    maxZErr = 0.01
    noDataVal = -9999.0

    # fill it with something
    for m in range(nBands):
        for i in range(nRows):
            for j in range(nCols):
                for k in range(nValuesPerPixel):
                    z = 0.001 * i * j + 5 * m + k
                    if j == i:    # for all values at the same pixel, will get pushed into the byte mask
                        z = noDataVal
                    if (m == 0 and i == 5 and j == 7 and k == 0):    # mixed case, decoded output will use noData for this one pixel in band 0
                        z = noDataVal
                    npArr[m][i][j][k] = z

    # prepare noData arrays
    npHasNoDataArr = np.full((nBands), True)    # input has noData values in all 3 bands
    npNoDataArr = np.zeros((nBands), 'd')    # noData value is always type double
    npNoDataArr.fill(noDataVal)    # noData value can vary between bands
    
    # encode
    nBytesBigEnough = npArr.nbytes * 2
    (result, numBytesWritten, outBuffer) = encode_4D(npArr, nValuesPerPixel, npValidMask, maxZErr, nBytesBigEnough, npHasNoDataArr, npNoDataArr, True)
    if result > 0:
        print('Error in encode_4D(): error code = ', result)
        return result
    print('num bytes written to buffer = ', numBytesWritten)

    # decode again
    (result, npArrDec, npValidMaskDec, npHasNoDataArrDec, npNoDataArrDec) = decode_4D(outBuffer, True)
    if result > 0:
        print('Error in test(): decode_4D() failed with error code = ', result)
        return result

    # evaluate the difference to orig
    maxZErrFound = findMaxZError_4D(npArr, npArrDec, npValidMaskDec, nBands)
    print('maxZErr found = ', maxZErrFound)

    # find the range [zMin, zMax]
    (zMin, zMax) = findDataRange_4D(npArrDec, npValidMaskDec, nBands, npHasNoDataArrDec, npNoDataArrDec)
    print('data range found = ', zMin, zMax)

    
    if False:
        print(' -------- decode test on ~100 different Lerc blobs -------- ')

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
                print('Error in test(): getLercBlobInfo() failed with error code = ', result)
                return result

            # read the data ranges, optional
            if nUsesNoData == 0:
                (result, npMins, npMaxs) = getLercDataRanges(bytesRead, nValuesPerPixel, nBands, False)
                if result > 0:
                    print('Error in test(): getLercDataRanges() failed with error code = ', result)
                    return result

            # decode
            (result, npArr, npValidMask, hasNoDataArr, noDataArr) = decode_4D(bytesRead, False)
            if result > 0:
                print('Error in test(): decode_4D() failed with error code = ', result)
                return result

            # find the range [zMin, zMax]
            (zMin, zMax) = findDataRange_4D(npArr, npValidMask, nBands, hasNoDataArr, noDataArr)
            
            print(f'codec {codecVersion:1}, dt {dataType:1}, nDepth {nValuesPerPixel:3}, nCols {nCols:5},',
                  f'nRows {nRows:5}, nBands {nBands:3}, nMasks {nMasks:3}, maxZErr {maxZErrUsed:.6f},',
                  f'nUsesNoData {nUsesNoData:3}, zMin {zMin:9.3f}, zMax {zMax:14.3f},  ', line.rstrip())

    return result
