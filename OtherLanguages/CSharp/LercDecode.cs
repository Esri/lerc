using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.IO;

namespace Lerc2015
{
    class LercDecode
    {
        const string lercDll = "../../../../bin/Lerc32.dll";

        [DllImport(lercDll)]
        public static extern UInt32 lerc_getBlobInfo(byte[] pLercBlob, UInt32 blobSize, UInt32[] infoArray, double[] dataRangeArray, int infoArraySize, int dataRangeArraySize);

        public enum DataType { dt_char, dt_uchar, dt_short, dt_ushort, dt_int, dt_uint, dt_float, dt_double }

        // Lerc decode functions for all Lerc compressed data types
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, byte[] pValidBytes, int nCols, int nRows, int nBands, int dataType, sbyte[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, byte[] pValidBytes, int nCols, int nRows, int nBands, int dataType, byte[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, byte[] pValidBytes, int nCols, int nRows, int nBands, int dataType, short[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, byte[] pValidBytes, int nCols, int nRows, int nBands, int dataType, ushort[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, byte[] pValidBytes, int nCols, int nRows, int nBands, int dataType, Int32[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, byte[] pValidBytes, int nCols, int nRows, int nBands, int dataType, UInt32[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, byte[] pValidBytes, int nCols, int nRows, int nBands, int dataType, float[] pData);
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, byte[] pValidBytes, int nCols, int nRows, int nBands, int dataType, double[] pData);

        // if you are lazy, don't want to deal with generic / templated code, and don't care about wasting memory: 
        // this function decodes the pixel values into a tile of data type double, independent of the compressed data type.
        [DllImport(lercDll)]
        public static extern UInt32 lerc_decodeToDouble(byte[] pLercBlob, UInt32 blobSize, byte[] pValidBytes, int nCols, int nRows, int nBands, double[] pData);

    }

    class GenericPixelLoop<T>
    {
        public static void GetMinMax(T[] pData, byte[] pValidBytes, int nCols, int nRows, int nBands)
        {
            double zMin = 1e30;
            double zMax = -zMin;

            // access the pixels; here, get the data range over all bands
            for (int iBand = 0; iBand < nBands; iBand++)
            {
                int k0 = nCols * nRows * iBand;
                for (int k = 0, i = 0; i < nRows; i++)
                    for (int j = 0; j < nCols; j++, k++)
                        if (1 == pValidBytes[k])    // pixel is valid
                        {
                            double z = Convert.ToDouble(pData[k0 + k]);
                            zMin = Math.Min(zMin, z);
                            zMax = Math.Max(zMax, z);
                        }
            }

            Console.WriteLine("[zMin, zMax] = [{0}, {1}]", zMin, zMax);
        }
    }

    class Program
    {
        static void Main(string[] args)
        {
            byte[] pLercBlob = File.ReadAllBytes(@"../../../../testData/california_400x400.lerc2");
            //byte[] pLercBlob = File.ReadAllBytes(@"../../../../testData/bluemarble_256_256_0.lerc2");

            String[] infoLabels = { "version", "data type", "nCols", "nRows", "nBands", "num valid pixels", "blob size" };
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
            int nCols = (int)infoArr[2];
            int nRows = (int)infoArr[3];
            int nBands = (int)infoArr[4];

            double zMin = dataRangeArr[0];
            double zMax = dataRangeArr[1];

            Console.WriteLine("[zMin, zMax] = [{0}, {1}]", zMin, zMax);

            byte[] pValidBytes = new byte[nCols * nRows];
            uint nValues = (uint)(nCols * nRows * nBands);

            Stopwatch sw = new Stopwatch();
            sw.Start();

            switch ((LercDecode.DataType)dataType)
            {
                case LercDecode.DataType.dt_char:
                    {
                        sbyte[] pData = new sbyte[nValues];
                        hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, pValidBytes, nCols, nRows, nBands, dataType, pData);
                        if (hr == 0)
                            GenericPixelLoop<sbyte>.GetMinMax(pData, pValidBytes, nCols, nRows, nBands);
                        break;
                    }
                case LercDecode.DataType.dt_uchar:
                    {
                        byte[] pData = new byte[nValues];
                        hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, pValidBytes, nCols, nRows, nBands, dataType, pData);
                        if (hr == 0)
                            GenericPixelLoop<byte>.GetMinMax(pData, pValidBytes, nCols, nRows, nBands);
                        break;
                    }
                case LercDecode.DataType.dt_short:
                    {
                        short[] pData = new short[nValues];
                        hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, pValidBytes, nCols, nRows, nBands, dataType, pData);
                        if (hr == 0)
                            GenericPixelLoop<short>.GetMinMax(pData, pValidBytes, nCols, nRows, nBands);
                        break;
                    }
                case LercDecode.DataType.dt_ushort:
                    {
                        ushort[] pData = new ushort[nValues];
                        hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, pValidBytes, nCols, nRows, nBands, dataType, pData);
                        if (hr == 0)
                            GenericPixelLoop<ushort>.GetMinMax(pData, pValidBytes, nCols, nRows, nBands);
                        break;
                    }
                case LercDecode.DataType.dt_int:
                    {
                        Int32[] pData = new Int32[nValues];
                        hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, pValidBytes, nCols, nRows, nBands, dataType, pData);
                        if (hr == 0)
                            GenericPixelLoop<Int32>.GetMinMax(pData, pValidBytes, nCols, nRows, nBands);
                        break;
                    }
                case LercDecode.DataType.dt_uint:
                    {
                        UInt32[] pData = new UInt32[nValues];
                        hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, pValidBytes, nCols, nRows, nBands, dataType, pData);
                        if (hr == 0)
                            GenericPixelLoop<UInt32>.GetMinMax(pData, pValidBytes, nCols, nRows, nBands);
                        break;
                    }
                case LercDecode.DataType.dt_float:
                    {
                        float[] pData = new float[nValues];
                        hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, pValidBytes, nCols, nRows, nBands, dataType, pData);
                        if (hr == 0)
                            GenericPixelLoop<float>.GetMinMax(pData, pValidBytes, nCols, nRows, nBands);
                        break;
                    }
                case LercDecode.DataType.dt_double:
                    {
                        double[] pData = new double[nValues];
                        hr = LercDecode.lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, pValidBytes, nCols, nRows, nBands, dataType, pData);
                        if (hr == 0)
                            GenericPixelLoop<double>.GetMinMax(pData, pValidBytes, nCols, nRows, nBands);
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
