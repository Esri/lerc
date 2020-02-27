#-------------------------------------------------------------------------------
#   Copyright 2016 - 2020 Esri
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
#   You need 2 files, this file "Lerc.py" and the Lerc dll (for Windows) or the
#   Lerc .so file (for Linux). Set the path below so the Lerc dll or .so file
#   can be read from here. You don't need a pip install or any installer. Then
#
#   >>> import Lerc
#   >>> Lerc.test()
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
            (nBands, nRows, nCols) = npShape
    elif nValuesPerPixel > 1:
        if dim == 3:
            (nRows, nCols, nValpp) = npShape
        elif dim == 4:
            (nBands, nRows, nCols, nValpp) = npShape
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

def findDataRange(npArr, npValidMask, nCols, nRows, nBands, nValidPixels, printInfo = False):
    start = timer()
    if (nValidPixels == nRows * nCols):
        zMin = np.amin(npArr)
        zMax = np.amax(npArr)
    else:
        if nBands == 1:
            zMin = np.amin(npArr[npValidMask])
            zMax = np.amax(npArr[npValidMask])
        elif nBands > 1:
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
lercDll.lerc_computeCompressedSize.argtypes = (ct.c_void_p, ct.c_uint, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_char_p, ct.c_double, ct.POINTER(ct.c_uint))

lercDll.lerc_encode.restype = ct.c_uint
lercDll.lerc_encode.argtypes = (ct.c_void_p, ct.c_uint, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_char_p, ct.c_double, ct.c_char_p, ct.c_uint, ct.POINTER(ct.c_uint))

lercDll.lerc_getBlobInfo.restype = ct.c_uint
lercDll.lerc_getBlobInfo.argtypes = (ct.c_char_p, ct.c_uint, ct.POINTER(ct.c_uint), ct.POINTER(ct.c_double), ct.c_int, ct.c_int)

lercDll.lerc_decode.restype = ct.c_uint
lercDll.lerc_decode.argtypes = (ct.c_char_p, ct.c_uint, ct.c_char_p, ct.c_int, ct.c_int, ct.c_int, ct.c_int, ct.c_uint, ct.c_void_p)

#-------------------------------------------------------------------------------

def encode(npArr, nValuesPerPixel, npValidMask, maxZErr, outBuffer, printInfo = False):
    global lercDll
    
    dataType = getLercDatatype(npArr.dtype)
    if dataType == -1:
        print('Error in encode(): unsupported numpy data type.')
        return (-1, 0)

    (nBands, nRows, nCols) = getLercShape(npArr, nValuesPerPixel)
    if nBands == 0:
        print('Error in encode(): unsupported numpy array shape.')
        return (-1, 0)

    if printInfo:
        print('dataType = ', dataType)
        print('nBands = ', nBands)
        print('nRows = ', nRows)
        print('nCols = ', nCols)
        print('nValuesPerPixel = ', nValuesPerPixel)
    
    byteArr = npArr.tobytes('C')  # C order
    cpData = ct.cast(byteArr, ct.c_void_p)
    npValidBytes = npValidMask.astype('B')
    validArr = npValidBytes.tobytes('C')
    cpValidArr = ct.cast(validArr, ct.c_char_p)
    ptr = ct.cast((ct.c_uint * 1)(), ct.POINTER(ct.c_uint))

    start = timer()
    
    if outBuffer == None:
        result = lercDll.lerc_computeCompressedSize(cpData, dataType, nValuesPerPixel, nCols, nRows, nBands, cpValidArr, maxZErr, ptr)
    else:
        result = lercDll.lerc_encode(cpData, dataType, nValuesPerPixel, nCols, nRows, nBands, cpValidArr, maxZErr, outBuffer, len(outBuffer), ptr)

    end = timer()
    
    if result > 0:
        print('Error in encode(): lercDll function failed with error code = ', result)
        return (result, 0)

    if printInfo:
        if outBuffer == None:
            print('time lerc_computeCompressedSize() = ', (end - start))
        else:
            print('time lerc_encode() = ', (end - start))
        
    return (result, ptr[0])

#-------------------------------------------------------------------------------

def getLercBlobInfo(compressedBytes, printInfo = False):
    global lercDll
    
    info = ['version', 'data type', 'nValuesPerPixel', 'nCols', 'nRows', 'nBands', 'nValidPixels', 'blob size']
    dataRange = ['zMin', 'zMax', 'maxZErrorUsed']
    nBytes = len(compressedBytes)
    len0 = len(info)
    len1 = len(dataRange)
    p0 = ct.cast((ct.c_uint * len0)(), ct.POINTER(ct.c_uint))
    p1 = ct.cast((ct.c_double * len1)(), ct.POINTER(ct.c_double))
    cpBytes = ct.cast(compressedBytes, ct.c_char_p)
    
    result = lercDll.lerc_getBlobInfo(cpBytes, nBytes, p0, p1, len0, len1)
    if result > 0:
        print('Error in getLercBlobInfo(): lercDLL.lerc_getBlobInfo() failed with error code = ', result)
        return (result, 0,0,0,0,0,0,0,0,0,0,0)

    if printInfo:
        for i in range(len0):
            print(info[i], p0[i])
        for i in range(len1):
            print(dataRange[i], p1[i])
    
    return (result, p0[0], p0[1], p0[2], p0[3], p0[4], p0[5], p0[6], p0[7], p1[0], p1[1], p1[2])

#-------------------------------------------------------------------------------

def decode(lercBlob, printInfo = False):
    global lercDll

    (result, version, dataType, nValuesPerPixel, nCols, nRows, nBands, nValidPixels, blobSize, zMin, zMax, maxZErrUsed) = getLercBlobInfo(lercBlob)
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

    # create empty buffer for valid pixels mask, if needed
    cpValidArr = None
    if nValidPixels != nRows * nCols:    # not all pixels are valid, need mask
        validBuf = ct.create_string_buffer(nRows * nCols)
        cpValidArr = ct.cast(validBuf, ct.c_char_p)

    # call decode
    start = timer()
    result = lercDll.lerc_decode(cpBytes, len(lercBlob), cpValidArr, nValuesPerPixel, nCols, nRows, nBands, dataType, cpData)
    end = timer()
    if result > 0:
        print('Error in decode(): lercDll.lerc_decode() failed with error code = ', result)
        return result
    
    if printInfo:
        print('time lerc_decode() = ', (end - start))

    # return result, np data array, and np valid pixels array if there
    npArr = np.frombuffer(dataBuf, npDtype)
    npArr.shape = shape

    if nValidPixels != nRows * nCols:
        npValidBytes = np.frombuffer(validBuf, dtype='B')
        npValidBytes.shape = (nRows, nCols)
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
    npValidMask = np.full((nRows, nCols), True)  # set all pixels valid
    maxZErr = 0.001

    # fill it with something
    for i in range(nRows):
        for j in range(nCols):
            for k in range(nValuesPerPixel):
                npArr[i][j][k] = 0.001 * i * j + k

    # call w/o outBuffer to only compute compressed size, optional
    (result, numBytes) = encode(npArr, nValuesPerPixel, npValidMask, maxZErr, None, True)
    if result > 0:
        print('Error in test(): error code = ', result)
        return result
    print('computed compressed size = ', numBytes)

    # encode
    outBuffer = '0' * numBytes    # or use buffer bigger than needed
    (result, numBytesWritten) = encode(npArr, nValuesPerPixel, npValidMask, maxZErr, outBuffer, True)
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
    maxZErrFound = findMaxZError(npArr, npArrDec)
    print('maxZErr found = ', maxZErrFound)

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

    # call w/o outBuffer to only compute compressed size, optional
    (result, numBytes) = encode(npArr, nValuesPerPixel, npValidMask, maxZErr, None, True)
    if result > 0:
        print('Error in encode(): error code = ', result)
        return result
    print('computed compressed size = ', numBytes)

    # encode
    outBuffer = '0' * numBytes
    (result, numBytesWritten) = encode(npArr, nValuesPerPixel, npValidMask, maxZErr, outBuffer, True)
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
    maxZErrFound = findMaxZError(npArr, npArrDec)
    print('maxZErr found = ', maxZErrFound)
    
    # save compressed Lerc blob to disk
    #open('C:/temp/test_1_256_256_3_double.lrc', 'wb').write(outBuffer)

    print(' -------- decode tests -------- ')

    folder = 'D:/GitHub/LercOpenSource/testData/'
    files = ['california_400_400_1_float.lerc2',
             'bluemarble_256_256_3_byte.lerc2',
             #'landsat_512_512_6_byte.lerc2',
             'world.lerc1']

    for n in range(len(files)):
        fn = folder + files[n]
        bytesRead = open(fn, 'rb').read()
        
        (result, version, dataType, nValuesPerPixel, nCols, nRows, nBands, nValidPixels, blobSize, zMin, zMax, maxZErrUsed) = getLercBlobInfo(bytesRead, True)
        if result > 0:
            print('Error in test(): getLercBlobInfo() failed with error code = ', result)
            return result

        (result, npArr, npValidMask) = decode(bytesRead, True)
        if result > 0:
            print('Error in test(): decode() failed with error code = ', result)
            return result

        # find the range [zMin, zMax] in the numpy array and compare to the above
        (zMin, zMax) = findDataRange(npArr, npValidMask, nCols, nRows, nBands, nValidPixels, True)
        print('data range found = ', zMin, zMax)
        print('------')
    
    return result
