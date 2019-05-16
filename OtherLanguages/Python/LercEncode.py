#-------------------------------------------------------------------------------
#   Copyright 2016 Esri
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

import array
import sys
from timeit import default_timer as timer
from ctypes import *

lercDll = CDLL ("D:/GitHub/LercOpenSource/bin/Windows/Lerc64.dll")  # windows
#lercDll = CDLL ("../../bin/Linux/Lerc64.so")  # linux

#-------------------------------------------------------------------------------
#
# see include/Lerc_c_api.h
#
#-------------------------------------------------------------------------------

lercDll.lerc_computeCompressedSize.restype = c_uint
lercDll.lerc_computeCompressedSize.argtypes = (c_void_p, c_uint, c_int, c_int, c_int, c_int, c_char_p, c_double, POINTER(c_uint))

def lercComputeCompressedSize(dataStr, dataType, nDim, nCols, nRows, nBands, cpValidBytes, maxZErr, pNumBytes):
    global lercDll
    start = timer()
    cpData = cast(dataStr, c_void_p)
    ptr = cast((c_uint * 1)(), POINTER(c_uint))
    
    result = lercDll.lerc_computeCompressedSize(cpData, dataType, nDim, nCols, nRows, nBands, cpValidBytes, maxZErr, ptr)
    
    pNumBytes[0] = ptr[0]
    end = timer()
    print('time lerc_computeCompressedSize() = ', (end - start))
    return result

#-------------------------------------------------------------------------------

lercDll.lerc_encode.restype = c_uint
lercDll.lerc_encode.argtypes = (c_void_p, c_uint, c_int, c_int, c_int, c_int, c_char_p, c_double, c_char_p, c_uint, POINTER(c_uint))

def lercEncode(dataStr, dataType, nDim, nCols, nRows, nBands, cpValidBytes, maxZErr, outBuffer, outBufferSize, pNumBytesWritten):
    global lercDll
    start = timer()
    cpData = cast(dataStr, c_void_p)
    ptr = cast((c_uint * 1)(), POINTER(c_uint))
    
    result = lercDll.lerc_encode(cpData, dataType, nDim, nCols, nRows, nBands, cpValidBytes, maxZErr, outBuffer, outBufferSize, ptr)
    
    pNumBytesWritten[0] = ptr[0]
    end = timer()
    print('time lerc_encode() = ', (end - start))
    return result

#-------------------------------------------------------------------------------

def lercEncodeFunction():

    # create a simple image tile sample to be encoded
    dataType = 7  # double
    nDim = 1
    nCols = 256
    nRows = 256
    nBands = 1
    cpValidMask = None
    maxZErr = 0.001
    pNumBytes = [1]
    dataArr = array.array('d', (0,) * nCols * nRows)

    for i in range(nRows):
        j0 = i * nCols
        for j in range(nCols):
            k0 = (j0 + j) * nDim
            for k in range(nDim):
                dataArr[k0 + k] = 0.001 * i * j

    dataStr = dataArr.tostring()  # pass as a string independent of data type
    result = lercComputeCompressedSize(dataStr, dataType, nDim, nCols, nRows, nBands, cpValidMask, maxZErr, pNumBytes)
    if result > 0:
        print('Error in lercComputeCompressedSize(): error code = ', result)
        return result

    numBytes = pNumBytes[0]
    print('computed compressed size = ', numBytes)
    
    outBuffer = create_string_buffer(numBytes)
    result = lercEncode(dataStr, dataType, nDim, nCols, nRows, nBands, cpValidMask, maxZErr, outBuffer, numBytes, pNumBytes)
    if result > 0:
        print('Error in lercEncode(): error code = ', result)
        return result

    numBytes = pNumBytes[0]
    print('num bytes written to buffer = ', numBytes)

    outFile = open("C:/temp/test_1_256_256_1_double.lrc", "wb")  # save compressed Lerc blob to disk
    outFile.write(outBuffer)

    return result

#-------------------------------------------------------------------------------

#>>> import sys
#>>> sys.path.append('D:/GitHub/LercOpenSource/OtherLanguages/Python')
#>>> import LercEncode
#>>> LercEncode.lercEncodeFunction()
