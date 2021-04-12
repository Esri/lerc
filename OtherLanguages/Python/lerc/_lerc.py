#-------------------------------------------------------------------------------
#   Copyright 2016 - 2021 Esri
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
#   How to use:
#
#   You need 2 files, this file "_lerc.py" and the Lerc dll (for Windows) or the
#   Lerc .so file (for Linux). Set the path below so the Lerc dll or .so file
#   can be read from here. You don't need a pip install or any installer. Then
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
dir_path = os.path.dirname(os.path.realpath(__file__))

if platform.system() == "Windows":
    lercDll = ct.CDLL (os.path.join(dir_path, 'Lerc.dll'))
if platform.system() == "Linux":
    lercDll = ct.CDLL (os.path.join(dir_path, 'Lerc.so'))
if platform.system() == "Darwin":
    lercDll = ct.CDLL (os.path.join(dir_path, 'Lerc.dylib'))

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

def findMaxZError(npArr1, npArr2):
    npDiff = npArr2 - npArr1
    yMin = np.amin(npDiff)
    yMax = np.amax(npDiff)
    return max(abs(yMin), abs(yMax))

#-------------------------------------------------------------------------------

def findDataRange(npArr, bHasMask, npValidMask, nBands, printInfo = False):
    start = timer()

    if not bHasMask:
        zMin = np.amin(npArr)
        zMax = np.amax(npArr)
    else:
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

#-------------------------------------------------------------------------------

# see include/Lerc_c_api.h

lercDll.lerc_computeCompressedSize.restype = ct.c_uint
lercDll.lerc_computeCompressedSize.argtypes = (ct.c_void_p, ct.c_uint, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_char_p, ct.c_double, ct.POINTER(ct.c_uint))

lercDll.lerc_encode.restype = ct.c_uint
lercDll.lerc_encode.argtypes = (ct.c_void_p, ct.c_uint, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_char_p, ct.c_double, ct.c_char_p, ct.c_uint, ct.POINTER(ct.c_uint))

lercDll.lerc_getBlobInfo.restype = ct.c_uint
lercDll.lerc_getBlobInfo.argtypes = (ct.c_char_p, ct.c_uint, ct.POINTER(ct.c_uint), ct.POINTER(ct.c_double), ct.c_int, ct.c_int)

lercDll.lerc_decode.restype = ct.c_uint
lercDll.lerc_decode.argtypes = (ct.c_char_p, ct.c_uint, ct.c_int, ct.c_char_p, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_uint, ct.c_void_p)

#-------------------------------------------------------------------------------

# npArr can be 2D, 3D, or 4D array. See also getLercShape() above.
#
# npValidMask can be None (bHasMask == False), 2D byte array, or 3D byte array (bHasMask == True).
# if 2D or [nRows, nCols], it is one mask for all bands. 1 means pixel is valid, 0 means invalid.
# if 3D or [nBands, nRows, nCols], it is one mask PER band.
# note that an array of values per pixel is either all valid or all invalid.
# the case that such inner array values are partially valid or invalid is not represented by a mask
# yet but a noData value would have to be used.
#
# nBytesHint can be
#  0 - compute num bytes needed for output buffer, but do not encode it (faster than encode)
#  1 - do both, compute exact buffer size needed and encode (slower than encode alone)
#  > 1 - create buffer of that given size and encode, if buffer too small encode will fail. 

def encode(npArr, nValuesPerPixel, bHasMask, npValidMask, maxZErr, nBytesHint, printInfo = False):
    global lercDll
    
    dataType = getLercDatatype(npArr.dtype)
    if dataType == -1:
        print('Error in encode(): unsupported numpy data type.')
        return (-1, 0)

    (nBands, nRows, nCols) = getLercShape(npArr, nValuesPerPixel)
    if nBands == 0:
        print('Error in encode(): unsupported numpy array shape.')
        return (-1, 0)

    nMasks = 0
    if bHasMask:
        (nMasks, nRows2, nCols2) = getLercShape(npValidMask, 1)
        if not(nMasks == 0 or nMasks == 1 or nMasks == nBands) or not(nRows2 == nRows and nCols2 == nCols):
            print('Error in encode(): unsupported mask array shape.')
            return (-1, 0)

    if printInfo:
        print('dataType = ', dataType)
        print('nBands = ', nBands)
        print('nRows = ', nRows)
        print('nCols = ', nCols)
        print('nValuesPerPixel = ', nValuesPerPixel)
        print('nMasks = ', nMasks)
    
    byteArr = npArr.tobytes('C')  # C order
    cpData = ct.cast(byteArr, ct.c_void_p)

    if bHasMask:
        npValidBytes = npValidMask.astype('B')
        validArr = npValidBytes.tobytes('C')
        cpValidArr = ct.cast(validArr, ct.c_char_p)
    else:
        cpValidArr = None

    ptr = ct.cast((ct.c_uint * 1)(), ct.POINTER(ct.c_uint))
    
    if nBytesHint == 0 or nBytesHint == 1:
        start = timer()
        result = lercDll.lerc_computeCompressedSize(cpData, dataType, nValuesPerPixel, nCols, nRows, nBands, nMasks, cpValidArr, maxZErr, ptr)
        nBytesNeeded = ptr[0]
        end = timer()

        if result > 0:
            print('Error in encode(): lercDll.lerc_computeCompressedSize() failed with error code = ', result)
            return (result, 0)

        if printInfo:
            print('time lerc_computeCompressedSize() = ', (end - start))
    else:
        nBytesNeeded = nBytesHint

    if nBytesHint > 0:
        outBytes = ct.create_string_buffer(nBytesNeeded)
        cpOutBuffer = ct.cast(outBytes, ct.c_char_p)
        start = timer()
        result = lercDll.lerc_encode(cpData, dataType, nValuesPerPixel, nCols, nRows, nBands, nMasks, cpValidArr, maxZErr, cpOutBuffer, nBytesNeeded, ptr)
        nBytesWritten = ptr[0]
        end = timer()

        if result > 0:
            print('Error in encode(): lercDll.lerc_encode() failed with error code = ', result)
            return (result, 0)

        if printInfo:
            print('time lerc_encode() = ', (end - start))

    if nBytesHint == 0:
        return (result, nBytesNeeded)
    else:
        return (result, nBytesWritten, outBytes)

#-------------------------------------------------------------------------------

def getLercBlobInfo(lercBlob, printInfo = False):
    global lercDll
    
    info = ['version', 'data type', 'nValuesPerPixel', 'nCols', 'nRows', 'nBands', 'nValidPixels', 'blob size', 'nMasks']
    dataRange = ['zMin', 'zMax', 'maxZErrorUsed']

    nBytes = len(lercBlob)
    len0 = len(info)
    len1 = len(dataRange)
    p0 = ct.cast((ct.c_uint * len0)(), ct.POINTER(ct.c_uint))
    p1 = ct.cast((ct.c_double * len1)(), ct.POINTER(ct.c_double))
    cpBytes = ct.cast(lercBlob, ct.c_char_p)
    
    result = lercDll.lerc_getBlobInfo(cpBytes, nBytes, p0, p1, len0, len1)
    if result > 0:
        print('Error in getLercBlobInfo(): lercDLL.lerc_getBlobInfo() failed with error code = ', result)
        return (result, 0,0,0,0,0,0,0,0,0,0,0,0)

    if printInfo:
        for i in range(len0):
            print(info[i], p0[i])
        for i in range(len1):
            print(dataRange[i], p1[i])
    
    return (result, p0[0], p0[1], p0[2], p0[3], p0[4], p0[5], p0[6], p0[7], p0[8], p1[0], p1[1], p1[2])

#-------------------------------------------------------------------------------

def decode(lercBlob, printInfo = False):
    global lercDll

    (result, version, dataType, nValuesPerPixel, nCols, nRows, nBands, nValidPixels, blobSize, nMasks, zMin, zMax, maxZErrUsed) = getLercBlobInfo(lercBlob, printInfo)
    if result > 0:
        print('Error in decode(): getLercBlobInfo() failed with error code = ', result)
        return result

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

    # call decode
    start = timer()
    result = lercDll.lerc_decode(cpBytes, len(lercBlob), nMasks, cpValidArr, nValuesPerPixel, nCols, nRows, nBands, dataType, cpData)
    end = timer()

    if result > 0:
        print('Error in decode(): lercDll.lerc_decode() failed with error code = ', result)
        return result
    
    if printInfo:
        print('time lerc_decode() = ', (end - start))

    # return result, np data array, and np valid pixels array if there
    npArr = np.frombuffer(dataBuf, npDtype)
    npArr.shape = shape

    if nMasks > 0:
        npValidBytes = np.frombuffer(validBuf, dtype='B')
        if nMasks == 1:
            npValidBytes.shape = (nRows, nCols)
        else:
            npValidBytes.shape = (nMasks, nRows, nCols)

        npValidMask = (npValidBytes != 0)
        return (result, npArr, npValidMask)
    else:
        return (result, npArr, None)
    
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

    # evaluate the difference to orig (assuming no mask all valid)
    maxZErrFound = findMaxZError(npArr, npArrDec)
    print('maxZErr found = ', maxZErrFound)

    # find the range [zMin, zMax] in the numpy array
    (zMin, zMax) = findDataRange(npArrDec, False, None, nBands, True)
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

    # evaluate the difference to orig (assuming no mask all valid)
    maxZErrFound = findMaxZError(npArr, npArrDec)
    print('maxZErr found = ', maxZErrFound)
    
    # save compressed Lerc blob to disk
    #open('C:/temp/test_1_256_256_3_double.lrc', 'wb').write(outBuffer)

    if False:
        print(' -------- decode tests -------- ')

        folder = 'D:/GitHub/LercOpenSource_v2.5/testData/'
        files = ['california_400_400_1_float.lerc2',
                 'bluemarble_256_256_3_byte.lerc2',
                 'landsat_512_512_6_byte.lerc2',
                 'world.lerc1',
                 'Different_Masks/lerc_level_0.lerc2']

        for n in range(len(files)):
            fn = folder + files[n]
            bytesRead = open(fn, 'rb').read()

            # read the blob header, optional
            (result, version, dataType, nValuesPerPixel, nCols, nRows, nBands, nValidPixels, blobSize, nMasks, zMin, zMax, maxZErrUsed) = getLercBlobInfo(bytesRead, False)
            if result > 0:
                print('Error in test(): getLercBlobInfo() failed with error code = ', result)
                return result

            # decode
            (result, npArr, npValidMask) = decode(bytesRead, True)
            if result > 0:
                print('Error in test(): decode() failed with error code = ', result)
                return result

            # find the range [zMin, zMax] in the numpy array and compare to the above
            (zMin, zMax) = findDataRange(npArr, nMasks > 0, npValidMask, nBands, True)
            print('data range found = ', zMin, zMax)
            print('------')
    
    return result
