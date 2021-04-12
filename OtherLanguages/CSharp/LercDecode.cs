/*
Copyright 2016 Esri

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
using System.Diagnostics;
using System.Linq;
using System.Runtime.InteropServices;
using System.IO;

namespace Lerc2017
{
    class LercDecode
    {
        const string lercDll = "Lerc.dll";

        // from Lerc_c_api.h :
        // 
        // typedef unsigned int lerc_status;
        //
        // // Call this function to get info about the compressed Lerc blob. Optional. 
        // // Info returned in infoArray is { version, dataType, nDim, nCols, nRows, nBands, nValidPixels, blobSize, nMasks }, see Lerc_types.h .
        // // Info returned in dataRangeArray is { zMin, zMax, maxZErrorUsed }, see Lerc_types.h .
        // // If more than 1 band the data range [zMin, zMax] is over all bands. 
        //
        // lerc_status lerc_getBlobInfo(const unsigned char* pLercBlob, unsigned int blobSize, 
        //   unsigned int* infoArray, double* dataRangeArray, int infoArraySize, int dataRangeArraySize);

        [DllImport(lercDll)]
        public static extern UInt32 lerc_getBlobInfo(byte[] pLercBlob, UInt32 blobSize, UInt32[] infoArray, double[] dataRangeArray, int infoArraySize, int dataRangeArraySize);

        public enum DataType { dt_char, dt_uchar, dt_short, dt_ushort, dt_int, dt_uint, dt_float, dt_double }

        // Lerc decode functions for all Lerc compressed data types

        // from Lerc_c_api.h :
        // 
        // // Decode the compressed Lerc blob into a raw data array.
        // // The data array must have been allocated to size (nDim * nCols * nRows * nBands * sizeof(dataType)).
        // // The valid bytes array, if not 0, must have been allocated to size (nCols * nRows * nMasks). 
        //
        // lerc_status lerc_decode(
        //   const unsigned char* pLercBlob,      // Lerc blob to decode
        //   unsigned int blobSize,               // blob size in bytes
        //   int nMasks,                          // 0, 1, or nBands; return as many masks in the next array
        //   unsigned char* pValidBytes,          // gets filled if not null ptr, even if all valid
        //   int nDim,                            // number of values per pixel
        //   int nCols, int nRows, int nBands,    // number of columns, rows, bands
        //   unsigned int dataType,               // data type of outgoing array
        //   void* pData);                        // outgoing data array

        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDim, int nCols, int nRows, int nBands, int dataType, sbyte[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDim, int nCols, int nRows, int nBands, int dataType, byte[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDim, int nCols, int nRows, int nBands, int dataType, short[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDim, int nCols, int nRows, int nBands, int dataType, ushort[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDim, int nCols, int nRows, int nBands, int dataType, Int32[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDim, int nCols, int nRows, int nBands, int dataType, UInt32[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDim, int nCols, int nRows, int nBands, int dataType, float[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDim, int nCols, int nRows, int nBands, int dataType, double[] pData);

        // if you are lazy, don't want to deal with generic / templated code, and don't care about wasting memory: 
        // this function decodes the pixel values into a tile of data type double, independent of the compressed data type.

        [DllImport(lercDll)]
        public static extern UInt32 lerc_decodeToDouble(byte[] pLercBlob, UInt32 blobSize, int nMasks, byte[] pValidBytes, int nDim, int nCols, int nRows, int nBands, double[] pData);
    }

    class GenericPixelLoop<T>
    {
        public static void GetMinMax(T[] pData, int nMasks, byte[] pValidBytes, int nDim, int nCols, int nRows, int nBands)
        {
            double zMin = 1e40;
            double zMax = -zMin;

            // access the pixels; here, get the data range over all bands
            for (int iBand = 0; iBand < nBands; iBand++)
            {
                int p0 = 0;
                if (nMasks > 1)
                    p0 = nCols * nRows * iBand;

                int k0 = nCols * nRows * iBand;
                for (int k = 0, i = 0; i < nRows; i++)
                    for (int j = 0; j < nCols; j++, k++)
                        if (0 == nMasks || 1 == pValidBytes[p0 + k])    // pixel is valid
                        {
                            for (int m = 0; m < nDim; m++)
                            {
                                double z = Convert.ToDouble(pData[(k0 + k) * nDim + m]);
                                zMin = Math.Min(zMin, z);
                                zMax = Math.Max(zMax, z);
                            }
                        }
            }

            Console.WriteLine("[zMin, zMax] = [{0}, {1}]", zMin, zMax);
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            //byte[] pLercBlob = File.ReadAllBytes(@"california_400_400_1_float.lerc2");
            //byte[] pLercBlob = File.ReadAllBytes(@"bluemarble_256_256_3_byte.lerc2");
            byte[] pLercBlob = File.ReadAllBytes(@"lerc_level_0.lerc2");

            String[] infoLabels = { "version", "data type", "nDim", "nCols", "nRows", "nBands", "num valid pixels", "blob size", "nMasks" };
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

            int lercVersion = (int)infoArr[0];
            int dataType = (int)infoArr[1];
            int nDim = (int)infoArr[2];
            int nCols = (int)infoArr[3];
            int nRows = (int)infoArr[4];
            int nBands = (int)infoArr[5];
            int nMasks = (int)infoArr[8];

            Console.WriteLine("[zMin, zMax] = [{0}, {1}]", dataRangeArr[0], dataRangeArr[1]);

            byte[] pValidBytes = new byte[nCols * nRows * nMasks];
            uint nValues = (uint)(nDim * nCols * nRows * nBands);

            Stopwatch sw = new Stopwatch();
            sw.Start();

            switch ((LercDecode.DataType)dataType)
            {
                case LercDecode.DataType.dt_char:
                    {
                        sbyte[] pData = new sbyte[nValues];
                        hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, nMasks, pValidBytes, nDim, nCols, nRows, nBands, dataType, pData);
                        if (hr == 0)
                            GenericPixelLoop<sbyte>.GetMinMax(pData, nMasks, pValidBytes, nDim, nCols, nRows, nBands);
                        break;
                    }
                case LercDecode.DataType.dt_uchar:
                    {
                        byte[] pData = new byte[nValues];
                        hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, nMasks, pValidBytes, nDim, nCols, nRows, nBands, dataType, pData);
                        if (hr == 0)
                            GenericPixelLoop<byte>.GetMinMax(pData, nMasks, pValidBytes, nDim, nCols, nRows, nBands);
                        break;
                    }
                case LercDecode.DataType.dt_short:
                    {
                        short[] pData = new short[nValues];
                        hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, nMasks, pValidBytes, nDim, nCols, nRows, nBands, dataType, pData);
                        if (hr == 0)
                            GenericPixelLoop<short>.GetMinMax(pData, nMasks, pValidBytes, nDim, nCols, nRows, nBands);
                        break;
                    }
                case LercDecode.DataType.dt_ushort:
                    {
                        ushort[] pData = new ushort[nValues];
                        hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, nMasks, pValidBytes, nDim, nCols, nRows, nBands, dataType, pData);
                        if (hr == 0)
                            GenericPixelLoop<ushort>.GetMinMax(pData, nMasks, pValidBytes, nDim, nCols, nRows, nBands);
                        break;
                    }
                case LercDecode.DataType.dt_int:
                    {
                        Int32[] pData = new Int32[nValues];
                        hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, nMasks, pValidBytes, nDim, nCols, nRows, nBands, dataType, pData);
                        if (hr == 0)
                            GenericPixelLoop<Int32>.GetMinMax(pData, nMasks, pValidBytes, nDim, nCols, nRows, nBands);
                        break;
                    }
                case LercDecode.DataType.dt_uint:
                    {
                        UInt32[] pData = new UInt32[nValues];
                        hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, nMasks, pValidBytes, nDim, nCols, nRows, nBands, dataType, pData);
                        if (hr == 0)
                            GenericPixelLoop<UInt32>.GetMinMax(pData, nMasks, pValidBytes, nDim, nCols, nRows, nBands);
                        break;
                    }
                case LercDecode.DataType.dt_float:
                    {
                        float[] pData = new float[nValues];
                        hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, nMasks, pValidBytes, nDim, nCols, nRows, nBands, dataType, pData);
                        if (hr == 0)
                            GenericPixelLoop<float>.GetMinMax(pData, nMasks, pValidBytes, nDim, nCols, nRows, nBands);
                        break;
                    }
                case LercDecode.DataType.dt_double:
                    {
                        double[] pData = new double[nValues];
                        hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, nMasks, pValidBytes, nDim, nCols, nRows, nBands, dataType, pData);
                        if (hr == 0)
                            GenericPixelLoop<double>.GetMinMax(pData, nMasks, pValidBytes, nDim, nCols, nRows, nBands);
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
            //Console.ReadKey();
        }
    }
}
