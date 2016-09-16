import ctypes
import array
from timeit import default_timer as timer

lercDll = ctypes.CDLL ("D:/GitHub/LercOpenSource/bin/Lerc32.dll")

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
#  // Decode the compressed Lerc blob into a double data array (independent of compressed data type). 
#  // The data array must have been allocated to size (nCols * nRows * nBands * sizeof(double)).
#  // The valid bytes array, if not 0, must have been allocated to size (nCols * nRows).
#  // Wasteful in memory, but convenient if a caller from C# or Python does not want to deal with 
#  // data type conversion, templating, or casting.
#
# lerc_status lerc_decodeToDouble(  const unsigned char* pLercBlob,
#                                   unsigned int blobSize,
#                                   unsigned char* pValidBytes,
#                                   int nCols,
#                                   int nRows,
#                                   int nBands,
#                                   double* pData);     // outgoing data array
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
    bytesRead = open("D:/GitHub/LercOpenSource/testData/california_400x400.lerc2", "rb").read()
    
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

    nCols = infoArr[2]
    nRows = infoArr[3]
    nBands = infoArr[4]
    nValidPixels = infoArr[5]

    cpValidMask = None
    
    if nValidPixels != nCols * nRows:    # not all pixels are valid, need mask
        cpValidMask = ctypes.create_string_buffer(nCols * nRows)
    
    cpData = ctypes.create_string_buffer(nCols * nRows * nBands * ctypes.sizeof(ctypes.c_double))
    cpData = ctypes.cast(cpData, ctypes.POINTER(ctypes.c_double))

    result = lercDecodeToDouble(bytesRead, cpValidMask, nCols, nRows, nBands, cpData)
    if result > 0:
        print ('Error in lercDecodeToDouble(): error code = ', result)
        return result

    # access the pixels: find the range [zMin, zMax] and compare to the above

    start = timer()
    
    zMin = +float("inf")
    zMax = -float("inf")

    if cpValidMask != None:     # not all pixels are valid, use the mask
        for m in range(nBands):
            for i in range(nRows):
                k = (m * nRows + i) * nCols
                for j in range(nCols):
                    if cpValidMask[k + j] != '\x00':
                        z = cpData[k + j]
                        zMin = min(zMin, z)
                        zMax = max(zMax, z)
                        
    else:                       # all pixels are valid, no mask needed
        for m in range(nBands):
            for i in range(nRows):
                k = (m * nRows + i) * nCols
                for j in range(nCols):
                    z = cpData[k + j]
                    zMin = min(zMin, z)
                    zMax = max(zMax, z)

    print ('data range found = ', zMin, zMax)

    end = timer()
    print ('time pixel loop in python: ', (end - start))
    
    return result


#>>> import sys
#>>> sys.path.append('D:/GitHub/LercOpenSource/OtherLanguages/Python')
#>>> import LercDecode
#>>> LercDecode.lercDecodeFunction()

