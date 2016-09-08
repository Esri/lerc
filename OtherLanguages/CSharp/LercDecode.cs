using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Runtime.InteropServices;
using System.IO;

namespace Lerc2015
{
    class Program
    {
        [DllImport("Lerc.dll")]
        public static extern UInt32 lerc_getBlobInfo(byte[] pLercBlob, UInt32 blobSize, UInt32[] infoArray, double[] dataRangeArray, int infoArraySize, int dataRangeArraySize);

        // if you know the compressed data type, such as float in elevation service, or uchar (= byte) for RGB image, 
        // then declare your image array of that type and call the proper decode function. 

        [DllImport("Lerc.dll")]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, byte[] pValidBytes, int nCols, int nRows, int nBands, int dataType, byte[] pData);
        [DllImport("Lerc.dll")]
        public static extern UInt32 lerc_decode(byte[] pLercBlob, UInt32 blobSize, byte[] pValidBytes, int nCols, int nRows, int nBands, int dataType, float[] pData);

        // if you don't know the compressed data type, the next function can be convenient: 
        // it reads the pixel values into a tile of data type double. 

        [DllImport("Lerc.dll")]
        public static extern UInt32 lerc_decodeToDouble(byte[] pLercBlob, UInt32 blobSize, byte[] pValidBytes, int nCols, int nRows, int nBands, double[] pData);

        enum LercDataType { dt_char, dt_uchar, dt_short, dt_ushort, dt_int, dt_uint, dt_float, dt_double };


        static void Main(string[] args)
        {
            byte[] pLercBlob = File.ReadAllBytes(@"testall_w1922_h1083_float.lerc2");

            UInt32[] infoArray = new UInt32[10];
            double[] dataRangeArray = new double[10];
            UInt32 hr = lerc_getBlobInfo(pLercBlob, (UInt32)pLercBlob.Length, infoArray, dataRangeArray, 10, 10);

            int lercVersion = (int)infoArray[0];
            int dataType = (int)infoArray[1];
            int nCols = (int)infoArray[2];
            int nRows = (int)infoArray[3];
            int nBands = (int)infoArray[4];

            double zMin = dataRangeArray[0];
            double zMax = dataRangeArray[1];

            Console.WriteLine("[zMin, zMax] = ");
            Console.WriteLine(zMin);
            Console.WriteLine(zMax);

            double min = 1e20;
            double max = -1e20;

            byte[] pValidBytes = new byte[nCols * nRows];

            if (dataType == (int)LercDataType.dt_uchar)
            {
                byte[] pData = new byte[nCols * nRows * nBands];
                hr = lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, pValidBytes, nCols, nRows, nBands, dataType, pData);

                // access the image data; here, get the data range over all bands
                for (int iBand = 0; iBand < nBands; iBand++)
                {
                    int k0 = nCols * nRows * iBand;
                    for (int k = 0, i = 0; i < nRows; i++)
                        for (int j = 0; j < nCols; j++, k++)
                            if (1 == pValidBytes[k])    // pixel is valid
                            {
                                double x = pData[k0 + k];
                                min = Math.Min(min, x);
                                max = Math.Max(max, x);
                            }
                }
            }
            else if (dataType == (int)LercDataType.dt_float)
            {
                float[] pData = new float[nCols * nRows * nBands];
                hr = lerc_decode(pLercBlob, (UInt32)pLercBlob.Length, pValidBytes, nCols, nRows, nBands, dataType, pData);

                // access the image data; here, get the data range over all bands
                for (int iBand = 0; iBand < nBands; iBand++)
                {
                    int k0 = nCols * nRows * iBand;
                    for (int k = 0, i = 0; i < nRows; i++)
                        for (int j = 0; j < nCols; j++, k++)
                            if (1 == pValidBytes[k])    // pixel is valid
                            {
                                double x = pData[k0 + k];
                                min = Math.Min(min, x);
                                max = Math.Max(max, x);
                            }
                }
            }
            else    // don't want to deal with compressed data type
            {
                double[] pData = new double[nCols * nRows * nBands];
                hr = lerc_decodeToDouble(pLercBlob, (UInt32)pLercBlob.Length, pValidBytes, nCols, nRows, nBands, pData);

                // access the image data; here, get the data range over all bands
                for (int iBand = 0; iBand < nBands; iBand++)
                {
                    int k0 = nCols * nRows * iBand;
                    for (int k = 0, i = 0; i < nRows; i++)
                        for (int j = 0; j < nCols; j++, k++)
                            if (1 == pValidBytes[k])    // pixel is valid
                            {
                                double x = pData[k0 + k];
                                min = Math.Min(min, x);
                                max = Math.Max(max, x);
                            }
                }
            }

            Console.WriteLine("[min, max] = ");
            Console.WriteLine(min);
            Console.WriteLine(max);

            Console.WriteLine("done");
            Console.ReadKey();
        }
    }
}
