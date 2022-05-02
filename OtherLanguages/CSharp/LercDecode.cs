/*
Copyright 2016-2022 Esri

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

Contributors:  Thomas Maurer, Wenxue Ju
*/

using System;
using System.Linq;
using System.Runtime.InteropServices;
using System.IO;
using System.Diagnostics;

namespace CSharpLercDecoder
{
    class LercDecode
    {
        const string lercDll = "Lerc.dll";

        // from Lerc_c_api.h :
        // 
        // typedef unsigned int lerc_status;
        //
        // // Call this to get info about the compressed Lerc blob. Optional.
        // // Info returned in infoArray is
        // // { version, dataType, nDepth, nCols, nRows, nBands, nValidPixels, blobSize, nMasks, nDepth, nUsesNoDataValue }, see Lerc_types.h .
        // // Info returned in dataRangeArray is { zMin, zMax, maxZErrorUsed }, see Lerc_types.h .
        // // If nDepth > 1 or nBands > 1 the data range [zMin, zMax] is over all values.
        //
        // lerc_status lerc_getBlobInfo(const unsigned char* pLercBlob, unsigned int blobSize, 
        //   unsigned int* infoArray, double* dataRangeArray, int infoArraySize, int dataRangeArraySize);

        [DllImport(lercDll)]
        public static extern UInt32 lerc_getBlobInfo(byte[] pLercBlob, UInt32 blobSize, UInt32[] infoArray, double[] dataRangeArray, int infoArraySize, int dataRangeArraySize);

        // from Lerc_c_api.h :
        // 
        // // Call this to quickly get the data ranges [min, max] per dimension and band without having to decode the pixels. Optional. 
        // // The 2 output data arrays must have been allocated to the same size (nDepth * nBands). 
        // // The output data array's layout is an image with nDepth columns and nBands rows. 
        //
        // lerc_status lerc_getDataRanges(const unsigned char* pLercBlob, unsigned int blobSize, int nDepth, int nBands, double* pMins, double* pMaxs);

        [DllImport(lercDll)]
        public static extern UInt32 lerc_getDataRanges(byte[] pLercBlob, UInt32 blobSize, int nDepth, int nBands, double[] mins, double[] maxs);

        public enum DataType { dt_char, dt_uchar, dt_short, dt_ushort, dt_int, dt_uint, dt_float, dt_double }

        // Lerc decode functions for all Lerc compressed data types

        // from Lerc_c_api.h :
        // 
        // // Decode the compressed Lerc blob into a raw data array.
        // // The data array must have been allocated to size (nDepth * nCols * nRows * nBands * sizeof(dataType)).
        // // The valid bytes array, if not 0, must have been allocated to size (nCols * nRows * nMasks). 
        //
        // lerc_status lerc_decode(
        //   const unsigned char* pLercBlob,      // Lerc blob to decode
        //   unsigned int blobSize,               // blob size in bytes
        //   int nMasks,                          // 0, 1, or nBands; return as many masks in the next array
        //   unsigned char* pValidBytes,          // gets filled if not null ptr, even if all valid
        //   int nDepth,                          // number of values per pixel
        //   int nCols, int nRows, int nBands,    // number of columns, rows, bands
        //   unsigned int dataType,               // data type of outgoing array
        //   void* pData);                        // outgoing data array

        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, int dataType, sbyte[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, int dataType, byte[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, int dataType, short[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, int dataType, ushort[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, int dataType, Int32[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, int dataType, UInt32[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, int dataType, float[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, int dataType, double[] pData);

        // if you are lazy, don't want to deal with generic / templated code, and don't care about wasting memory: 
        // this function decodes the pixel values into a tile of data type double, independent of the compressed data type.

        [DllImport(lercDll)]
        public static extern UInt32 lerc_decodeToDouble(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, double[] pData);

        // Lerc decode functions for all Lerc compressed data types, extended to take one noData value per band if one is used in that band.
        // This covers the case of an array per pixel (nDepth > 1), and a mix of valid and invalid values at the same pixel. 

        // from Lerc_c_api.h :
        //
        // // Same as for regular decode, first call lerc_getBlobInfo() to get all info needed from the blob header. 
        // // Check the property (InfoArray::nUsesNoDataValue) to check if there is any noData value used. 
        // //
        // // If not, just pass nullptr for the last 2 arguments. 
        // //
        // // If yes, pass 2 arrays of size nBands each, one value per band. 
        // // In pUsesNoData array, for each band, 1 means a noData value is used, 0 means not.
        // // In noDataValues array, for each band, it has the noData value if there is one. 
        // // This is the same noData value as passed for encode. 

        //lerc_status lerc_decode_4D(
        //  const unsigned char* pLercBlob,    // Lerc blob to decode
        //  unsigned int blobSize,             // blob size in bytes
        //  int nMasks,                        // 0, 1, or nBands; return as many masks in the next array
        //  unsigned char* pValidBytes,        // gets filled if not null ptr, even if all valid
        //  int nDepth,                        // number of values per pixel (e.g., 3 for RGB, data is stored as [RGB, RGB, ...])
        //  int nCols,                         // number of columns
        //  int nRows,                         // number of rows
        //  int nBands,                        // number of bands (e.g., 3 for [RRRR ..., GGGG ..., BBBB ...])
        //  unsigned int dataType,             // char = 0, uchar = 1, short = 2, ushort = 3, int = 4, uint = 5, float = 6, double = 7
        //  void* pData,                       // outgoing data array
        //  unsigned char* pUsesNoData,        // pass an array of size nBands, 1 - band uses noData, 0 - not
        //  double* noDataValues);             // same, pass an array of size nBands to get the noData value per band, if any

        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode_4D(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, int dataType, sbyte[] pData, byte[] pUsesNoData, double[] noDataValues);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode_4D(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, int dataType, byte[] pData, byte[] pUsesNoData, double[] noDataValues);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode_4D(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, int dataType, short[] pData, byte[] pUsesNoData, double[] noDataValues);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode_4D(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, int dataType, ushort[] pData, byte[] pUsesNoData, double[] noDataValues);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode_4D(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, int dataType, Int32[] pData, byte[] pUsesNoData, double[] noDataValues);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode_4D(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, int dataType, UInt32[] pData, byte[] pUsesNoData, double[] noDataValues);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode_4D(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, int dataType, float[] pData, byte[] pUsesNoData, double[] noDataValues);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode_4D(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, int dataType, double[] pData, byte[] pUsesNoData, double[] noDataValues);


        [DllImport(lercDll)]
        public static extern UInt32 lerc_decodeToDouble_4D(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, double[] pData, byte[] pUsesNoData, double[] noDataValues);
    }

    class GenericPixelLoop<T>
    {
        public static void GetMinMax(T[] pData, int nMasks, byte[] pValidBytes, int nDepth, int nCols, int nRows, int nBands, byte[] pUsesNoData = null, double[] noDataValues = null)
        {
            double zMin = 1e40;
            double zMax = -zMin;

            // access the pixels; here, get the data range over all bands
            for (int iBand = 0; iBand < nBands; iBand++)
            {
                // get the noData value for this band, if any
                bool bHasNoData = (pUsesNoData != null) && (pUsesNoData[iBand] > 0);
                double noDataVal = bHasNoData ? noDataValues[iBand] : 0;

                int p0 = 0;
                if (nMasks > 1)
                    p0 = nCols * nRows * iBand;

                int k0 = nCols * nRows * iBand;
                for (int k = 0, i = 0; i < nRows; i++)
                    for (int j = 0; j < nCols; j++, k++)
                        if (0 == nMasks || 1 == pValidBytes[p0 + k])    // pixel is valid
                        {
                            for (int m = 0; m < nDepth; m++)
                            {
                                double z = Convert.ToDouble(pData[(k0 + k) * nDepth + m]);

                                // honor the noData value for this band, if there
                                if (bHasNoData && z == noDataVal)
                                    continue;

                                zMin = Math.Min(zMin, z);
                                zMax = Math.Max(zMax, z);
                            }
                        }
            }

            Console.WriteLine("[zMin, zMax] = [{0}, {1}]", zMin, zMax);
        }
    }

    internal class Program
    {
        static void Main(string[] args)
        {
            //byte[] pLercBlob = File.ReadAllBytes(@"california_400_400_1_float.lerc2");
            //byte[] pLercBlob = File.ReadAllBytes(@"bluemarble_256_256_3_byte.lerc2");
            byte[] pLercBlob = File.ReadAllBytes(@"lerc_level_0.lerc2");

            // get blob info

            String[] infoLabels = { "codec version", "data type", "nDepth", "nCols", "nRows", "nBands", 
                "num valid pixels", "blob size", "nMasks", "nDepth", "nUsesNoDataValue" };

            String[] dataRangeLabels = { "zMin", "zMax", "maxZErrorUsed" };

            int infoArrSize = infoLabels.Count();
            int dataRangeArrSize = dataRangeLabels.Count();

            UInt32[] infoArr = new UInt32[infoArrSize];
            double[] dataRangeArr = new double[dataRangeArrSize];

            UInt32 hr = LercDecode.lerc_getBlobInfo(pLercBlob, (UInt32)pLercBlob.Length, infoArr, dataRangeArr, infoArrSize, dataRangeArrSize);
            if (hr > 0)
            {
                Console.WriteLine("function lerc_getBlobInfo(...) failed with error code {0}.", hr);
                return;
            }

            Console.WriteLine("Lerc blob info:");
            for (int i = 0; i < infoArrSize; i++)
                Console.WriteLine("{0} = {1}", infoLabels[i], infoArr[i]);
            for (int i = 0; i < dataRangeArrSize; i++)
                Console.WriteLine("{0} = {1}", dataRangeLabels[i], dataRangeArr[i]);

            int lercCodecVersion = (int)infoArr[0];
            int dataType = (int)infoArr[1];
            int nDepth = (int)infoArr[2];
            int nCols = (int)infoArr[3];
            int nRows = (int)infoArr[4];
            int nBands = (int)infoArr[5];
            int nMasks = (int)infoArr[8];

            int nUsesNoData = (int)infoArr[10];

            if (nUsesNoData == 0)    // "normal" case, the byte masks fully represent invalid values, no additional noData value is used
            {
                Console.WriteLine("[zMin, zMax] = [{0}, {1}]\n", dataRangeArr[0], dataRangeArr[1]);

                // get data ranges per dimension and band

                Stopwatch sw = new Stopwatch();
                sw.Start();

                double[] mins = new double[nDepth * nBands];
                double[] maxs = new double[nDepth * nBands];

                hr = LercDecode.lerc_getDataRanges(pLercBlob, (UInt32)pLercBlob.Length, nDepth, nBands, mins, maxs);
                if (hr > 0)
                {
                    Console.WriteLine("function lerc_getDataRanges(...) failed with error code {0}.", hr);
                    return;
                }

                sw.Stop();

                Console.WriteLine("Data ranges read from Lerc headers:");
                for (int i = 0; i < nBands; i++)
                    for (int j = 0; j < nDepth; j++)
                        Console.WriteLine("Band {0}, depth {1}: [{2}, {3}]", i, j, mins[i * nDepth + j], maxs[i * nDepth + j]);

                Console.WriteLine("total time for lerc_getDataRanges(...) = {0} ms\n", sw.ElapsedMilliseconds);


                // decode the pixel values

                byte[] pValidBytes = new byte[nCols * nRows * nMasks];
                uint nValues = (uint)(nDepth * nCols * nRows * nBands);

                sw.Start();

                switch ((LercDecode.DataType)dataType)
                {
                    case LercDecode.DataType.dt_char:
                        {
                            sbyte[] pData = new sbyte[nValues];
                            hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, nMasks, pValidBytes, nDepth, nCols, nRows, nBands, dataType, pData);
                            if (hr == 0)
                                GenericPixelLoop<sbyte>.GetMinMax(pData, nMasks, pValidBytes, nDepth, nCols, nRows, nBands);
                            break;
                        }
                    case LercDecode.DataType.dt_uchar:
                        {
                            byte[] pData = new byte[nValues];
                            hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, nMasks, pValidBytes, nDepth, nCols, nRows, nBands, dataType, pData);
                            if (hr == 0)
                                GenericPixelLoop<byte>.GetMinMax(pData, nMasks, pValidBytes, nDepth, nCols, nRows, nBands);
                            break;
                        }
                    case LercDecode.DataType.dt_short:
                        {
                            short[] pData = new short[nValues];
                            hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, nMasks, pValidBytes, nDepth, nCols, nRows, nBands, dataType, pData);
                            if (hr == 0)
                                GenericPixelLoop<short>.GetMinMax(pData, nMasks, pValidBytes, nDepth, nCols, nRows, nBands);
                            break;
                        }
                    case LercDecode.DataType.dt_ushort:
                        {
                            ushort[] pData = new ushort[nValues];
                            hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, nMasks, pValidBytes, nDepth, nCols, nRows, nBands, dataType, pData);
                            if (hr == 0)
                                GenericPixelLoop<ushort>.GetMinMax(pData, nMasks, pValidBytes, nDepth, nCols, nRows, nBands);
                            break;
                        }
                    case LercDecode.DataType.dt_int:
                        {
                            Int32[] pData = new Int32[nValues];
                            hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, nMasks, pValidBytes, nDepth, nCols, nRows, nBands, dataType, pData);
                            if (hr == 0)
                                GenericPixelLoop<Int32>.GetMinMax(pData, nMasks, pValidBytes, nDepth, nCols, nRows, nBands);
                            break;
                        }
                    case LercDecode.DataType.dt_uint:
                        {
                            UInt32[] pData = new UInt32[nValues];
                            hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, nMasks, pValidBytes, nDepth, nCols, nRows, nBands, dataType, pData);
                            if (hr == 0)
                                GenericPixelLoop<UInt32>.GetMinMax(pData, nMasks, pValidBytes, nDepth, nCols, nRows, nBands);
                            break;
                        }
                    case LercDecode.DataType.dt_float:
                        {
                            float[] pData = new float[nValues];
                            hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, nMasks, pValidBytes, nDepth, nCols, nRows, nBands, dataType, pData);
                            if (hr == 0)
                                GenericPixelLoop<float>.GetMinMax(pData, nMasks, pValidBytes, nDepth, nCols, nRows, nBands);
                            break;
                        }
                    case LercDecode.DataType.dt_double:
                        {
                            double[] pData = new double[nValues];
                            hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, nMasks, pValidBytes, nDepth, nCols, nRows, nBands, dataType, pData);
                            if (hr == 0)
                                GenericPixelLoop<double>.GetMinMax(pData, nMasks, pValidBytes, nDepth, nCols, nRows, nBands);
                            break;
                        }
                }

                if (hr > 0)
                {
                    Console.WriteLine("function lerc_decode(...) failed with error code {0}.", hr);
                    return;
                }

                sw.Stop();
                Console.WriteLine("total time for Lerc decode and C# pixel loop = {0} ms", sw.ElapsedMilliseconds);
                Console.ReadKey();
            }

            else if (nUsesNoData > 0)
            {
                // nDepth > 1, array per pixel, and in 1 or more bands there is a mix of valid and invalid values at the same pixel,
                // so the byte masks alone do not fully represent all invalid values: noData values must be honored, too. 

                byte[] pValidBytes = new byte[nCols * nRows * nMasks];
                uint nValues = (uint)(nDepth * nCols * nRows * nBands);

                double[] pData = new double[nValues];
                byte[] pUsesNoData = new byte[nBands];
                double[] noDataValues = new double[nBands];

                hr = LercDecode.lerc_decodeToDouble_4D(pLercBlob, (UInt32)pLercBlob.Length, nMasks, pValidBytes, nDepth, nCols, nRows, nBands,
                    pData, pUsesNoData, noDataValues);

                if (hr == 0)
                    GenericPixelLoop<double>.GetMinMax(pData, nMasks, pValidBytes, nDepth, nCols, nRows, nBands, pUsesNoData, noDataValues);

                if (hr > 0)
                {
                    Console.WriteLine("function lerc_decodeToDouble_4D(...) failed with error code {0}.", hr);
                    return;
                }
            }
        }
    }
}
