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
import ctypes
import sys
from timeit import default_timer as timer

lercDll = ctypes.CDLL ("D:/GitHub/LercOpenSource/bin/Windows/Lerc32.dll")    # windows
#lercDll = ctypes.CDLL ("../../bin/Linux/Lerc32.so")    # linux

#-------------------------------------------------------------------------------
#
# from Lerc_c_api.h :
#
# typedef unsigned int lerc_status;
#
# // Call this to get info about the compressed Lerc blob. Optional. 
# // Info returned in infoArray is { version, dataType, nCols, nRows, nBands, nValidPixels, blobSize },
# // see Lerc_types.h .
# // Info returned in dataRangeArray is { zMin, zMax, maxZErrorUsed }, see Lerc_types.h .
#
# lerc_status lerc_getBlobInfo( const unsigned char* pLercBlob,
#                               unsigned int blobSize, 
#                               unsigned int* infoArray,
#                               double* dataRangeArray,
#                               int infoArraySize,
#                               int dataRangeArraySize);
#
#-------------------------------------------------------------------------------

lercDll.lerc_getBlobInfo.restype = ctypes.c_uint

lercDll.lerc_getBlobInfo.argtypes = (ctypes.c_char_p,
                                     ctypes.c_uint,
                                     ctypes.POINTER(ctypes.c_uint),
                                     ctypes.POINTER(ctypes.c_double),
                                     ctypes.c_int,
                                     ctypes.c_int)

def lercGetBlobInfo(compressedBytes, infoArr, dataRangeArr):
    global lercDll
    
    nBytes = len(compressedBytes)
    len0 = min(7, len(infoArr))
    len1 = min(3, len(dataRangeArr))
        
    arrTp0 = (ctypes.c_uint * len0)()
    arrTp1 = (ctypes.c_double * len1)()
    
    ptr0 = ctypes.cast(arrTp0, ctypes.POINTER(ctypes.c_uint))
    ptr1 = ctypes.cast(arrTp1, ctypes.POINTER(ctypes.c_double))
    
    result = lercDll.lerc_getBlobInfo(ctypes.c_char_p(compressedBytes),
                                      ctypes.c_uint(nBytes),
                                      ptr0,
                                      ptr1,
                                      ctypes.c_int(len0),
                                      ctypes.c_int(len1))
    if result > 0:
        return result
    
    for i in range(len0):
        infoArr[i] = ptr0[i]

    for i in range(len1):
        dataRangeArr[i] = ptr1[i]

    return result


#-------------------------------------------------------------------------------
#
# // Decode the compressed Lerc blob into a raw data array.
# // The data array must have been allocated to size (nCols * nRows * nBands * sizeof(dataType)).
# // The valid bytes array, if not 0, must have been allocated to size (nCols * nRows).
#
# lerc_status lerc_decode(  const unsigned char* pLercBlob,
#                           unsigned int blobSize,
#                           unsigned char* pValidBytes,
#                           int nCols,
#                           int nRows,
#                           int nBands,
#                           unsigned int dataType,    // data type of outgoing array
#                           void* pData);             // outgoing data array
#
#-------------------------------------------------------------------------------

lercDll.lerc_decode.restype = ctypes.c_uint

lercDll.lerc_decode.argtypes = (ctypes.c_char_p,
                                ctypes.c_uint,
                                ctypes.c_char_p,
                                ctypes.c_int,
                                ctypes.c_int,
                                ctypes.c_int,
                                ctypes.c_uint,
                                ctypes.c_void_p)

def lercDecode(lercBlob, cpValidBytes, nCols, nRows, nBands, dataType, cpData):
    global lercDll
    start = timer()
    result = lercDll.lerc_decode(ctypes.c_char_p(lercBlob),
                                 ctypes.c_uint(len(lercBlob)),
                                 cpValidBytes,
                                 ctypes.c_int(nCols),
                                 ctypes.c_int(nRows),
                                 ctypes.c_int(nBands),
                                 ctypes.c_uint(dataType),
                                 cpData)
    end = timer()
    print ('time lerc decode: ', (end - start))
    return result

#-------------------------------------------------------------------------------
#
# // Decode the compressed Lerc blob into a double data array (independent of compressed data type). 
# // The data array must have been allocated to size (nCols * nRows * nBands * sizeof(double)).
# // The valid bytes array, if not 0, must have been allocated to size (nCols * nRows).
# // Wasteful in memory, but convenient if a caller from C# or Python does not want to deal with 
# // data type conversion, templating, or casting.
#
# lerc_status lerc_decodeToDouble(  const unsigned char* pLercBlob,
#                                   unsigned int blobSize,
#                                   unsigned char* pValidBytes,
#                                   int nCols,
#                                   int nRows,
#                                   int nBands,
#                                   double* pData);    // outgoing data array
#
#-------------------------------------------------------------------------------

lercDll.lerc_decodeToDouble.restype = ctypes.c_uint

lercDll.lerc_decodeToDouble.argtypes = (ctypes.c_char_p,
                                        ctypes.c_uint,
                                        ctypes.c_char_p,
                                        ctypes.c_int,
                                        ctypes.c_int,
                                        ctypes.c_int,
                                        ctypes.POINTER(ctypes.c_double))

def lercDecodeToDouble(lercBlob, cpValidBytes, nCols, nRows, nBands, cpData):
    global lercDll
    start = timer()
    result = lercDll.lerc_decodeToDouble(ctypes.c_char_p(lercBlob),
                                         ctypes.c_uint(len(lercBlob)),
                                         cpValidBytes,
                                         ctypes.c_int(nCols),
                                         ctypes.c_int(nRows),
                                         ctypes.c_int(nBands),
                                         cpData)
    end = timer()
    print ('time lerc decode: ', (end - start))
    return result


#-------------------------------------------------------------------------------

def lercDecodeFunction():
    bytesRead = open("D:/GitHub/LercOpenSource/testData/california_400_400_1_float.lerc2", "rb").read()
    #bytesRead = open("D:/GitHub/LercOpenSource/testData/bluemarble_256_256_3_byte.lerc2", "rb").read()
    #bytesRead = open("D:/GitHub/LercOpenSource/testData/landsat_512_512_6_byte.lerc2", "rb").read()
    #bytesRead = open("D:/GitHub/LercOpenSource/py/world.lerc", "rb").read()

    infoArr = array.array('L', (0,) * 7)
    dataRangeArr = array.array('d', (0,) * 3)

    result = lercGetBlobInfo(bytesRead, infoArr, dataRangeArr)
    if result > 0:
        return result

    info = ['version', 'data type', 'nCols', 'nRows', 'nBands', 'nValidPixels', 'blob size']
    for i in range(len(infoArr)):
        print (info[i], infoArr[i])

    dataRange = ['zMin', 'zMax', 'maxZErrorUsed']
    for i in range(len(dataRangeArr)):
        print (dataRange[i], dataRangeArr[i])

    version = infoArr[0]
    dataType = infoArr[1]
    nCols = infoArr[2]
    nRows = infoArr[3]
    nBands = infoArr[4]
    nValidPixels = infoArr[5]

    cpValidMask = None
    c00 = '\x00'
    
    if nValidPixels != nCols * nRows:    # not all pixels are valid, need mask
        cpValidMask = ctypes.create_string_buffer(nCols * nRows)


    # Here we show 2 options for Lerc decode, lercDecode() and lercDecodeToDouble().
    # We use the first for the integer types, the second for the floating point types.
    # This is for illustration only. You can use each decode function on all data types.
    # lercDecode() is closer to the native data types, uses less memory, but you need
    # to cast the pointers around.
    # lercDecodeToDouble() is more convenient to call, but a waste of memory esp if
    # the compressed data type is only byte or char. 

    if dataType < 6:    # integer types  [char, uchar, short, ushort, int, uint]
        
        if dataType == 0 or dataType == 1:      # char or uchar
            sizeOfData = 1
        elif dataType == 2 or dataType == 3:    # short or ushort
            sizeOfData = 2
        else:
            sizeOfData = 4

        dataStr = ctypes.create_string_buffer(nCols * nRows * nBands * sizeOfData)
        cpData = ctypes.cast(dataStr, ctypes.c_void_p)

        result = lercDecode(bytesRead, cpValidMask, nCols, nRows, nBands, dataType, cpData)
        if result > 0:
            print ('Error in lercDecode(): error code = ', result)
            return result

        # cast to proper pointer type
        
        if dataType == 0:
            cpData = ctypes.cast(dataStr, ctypes.c_char_p)
        elif dataType == 1:
            cpData = ctypes.cast(dataStr, ctypes.POINTER(ctypes.c_ubyte))
        elif dataType == 2:
            cpData = ctypes.cast(dataStr, ctypes.POINTER(ctypes.c_short))
        elif dataType == 3:
            cpData = ctypes.cast(dataStr, ctypes.POINTER(ctypes.c_ushort))
        elif dataType == 4:
            cpData = ctypes.cast(dataStr, ctypes.POINTER(ctypes.c_int))
        elif dataType == 5:
            cpData = ctypes.cast(dataStr, ctypes.POINTER(ctypes.c_uint))

        zMin = sys.maxint
        zMax = -zMin


    else:    # floating point types  [float, double]
        
        dataStr = ctypes.create_string_buffer(nCols * nRows * nBands * ctypes.sizeof(ctypes.c_double))
        cpData = ctypes.cast(dataStr, ctypes.POINTER(ctypes.c_double))

        result = lercDecodeToDouble(bytesRead, cpValidMask, nCols, nRows, nBands, cpData)
        if result > 0:
            print ('Error in lercDecodeToDouble(): error code = ', result)
            return result

        # btw you can convert the ctypes arrays back into regular python arrays,
        # but not much advantage here for the price of an extra copy
        
        #validByteArr = array.array('b')
        #validByteArr.fromstring(cpValidMask)
        #dataArr = array.array('d')
        #dataArr.fromstring(dataStr)

        zMin = float("inf")
        zMax = -zMin


    # access the pixels: find the range [zMin, zMax] and compare to the above

    start = timer()
    
    for m in range(nBands):
        i0 = m * nRows * nCols
        for i in range(nRows):
            j0 = i * nCols
            if cpValidMask != None:    # not all pixels are valid, use the mask
                for j in range(nCols):
                    if cpValidMask[j0 + j] != c00:
                        z = cpData[i0 + j0 + j]
                        zMin = min(zMin, z)
                        zMax = max(zMax, z)
            else:                      # all pixels are valid, no mask needed
                for j in range(nCols):
                    z = cpData[i0 + j0 + j]
                    zMin = min(zMin, z)
                    zMax = max(zMax, z)

    end = timer()
    
    print ('data range found = ', zMin, zMax)
    print ('time pixel loop in python: ', (end - start))
    
    return result


#>>> import sys
#>>> sys.path.append('D:/GitHub/LercOpenSource/OtherLanguages/Python')
#>>> import LercDecode
#>>> LercDecode.lercDecodeFunction()

