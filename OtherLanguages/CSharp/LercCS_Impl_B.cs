// Alternative implementation for Lerc Encoding/Decoding in C#.
// By Ákos Halmai 2024.


using Microsoft.Win32.SafeHandles;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace LercNS
{
    public enum ErrorCode : int
    {
        Ok = 0,
        Failed,
        WrongParam,
        BufferTooSmall,
        NaN,
        HasNoData
    };

    public enum DataType : uint
    {
        SByte = 0,
        Byte,
        Short,
        UShort,
        Int,
        UInt,
        Float,
        Double
    };

    public enum MaskSettings : int
    {
        AllValid,
        SameMaskForAllBands,
        UniqueMaskForEveryBand
    };

    [SkipLocalsInit]
    [StructLayout(LayoutKind.Sequential, Pack = 1, Size = Size)]
    public readonly struct LercBlobInfo
    {
        public const int Count = 11;
        public const int Size = Count * sizeof(int);
        public readonly uint Version { get; }
        public readonly DataType DataType { get; }
        public readonly uint NDim { get; }
        public readonly uint NCols { get; }
        public readonly uint NRows { get; }
        public readonly uint NBands { get; }
        public readonly uint NValidPixels { get; }
        public readonly uint BlobSize { get; }
        public readonly uint NMasks { get; }
        public readonly uint NDepth { get; }
        private readonly uint UsesNoDataValueInternal { get; }
        public readonly bool UsesNoDataValue => UsesNoDataValueInternal is not 0;

        public readonly uint OutputBlobSize
        {
            [SkipLocalsInit]
            get
            {
                return NDepth * NCols * NRows * NBands * (DataType switch
                {
                    DataType.Byte => sizeof(byte),
                    DataType.SByte => sizeof(sbyte),
                    DataType.Short => sizeof(short),
                    DataType.UShort => sizeof(ushort),
                    DataType.Int => sizeof(int),
                    DataType.UInt => sizeof(uint),
                    DataType.Float => sizeof(float),
                    DataType.Double => sizeof(double),
                    _ => ThrowUnsupportedDataType()
                });

                [DoesNotReturn]
                static uint ThrowUnsupportedDataType() =>
                 throw new Exception("Unsupported data type.");
            }
        }

        public readonly uint ByteMaskSize
        {
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            [SkipLocalsInit]
            get { return NCols * NRows * NMasks; }
        }
    };

    [SkipLocalsInit]
    [StructLayout(LayoutKind.Sequential, Pack = 1, Size = Size)]
    public readonly struct DataRangeInfo
    {
        public const int Count = 3;
        public const int Size = Count * sizeof(double);
        public readonly double ZMin { get; }
        public readonly double ZMax { get; }
        public readonly double MaxZErrorUsed { get; }
    };
    public static class LercCS
    {
        [NotNull]
        [DisallowNull]
        public const string LercDLL = @"Lerc.dll";

        public static bool IsLercDLLAvailable
        {
            [SkipLocalsInit]
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => File.Exists(LercDLL);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining), SkipLocalsInit]
        public static unsafe uint ComputeCompressedSize<T>([DisallowNull][NotNull] ReadOnlySpan<T> data,
                                                        int nDepth,
                                                        int nCols,
                                                        int nRows,
                                                        int nBands,
                                                        MaskSettings masks = MaskSettings.AllValid,
                                                        double maxZErr = 0d,
                                                        ReadOnlySpan<byte> validBytes = default) where T : unmanaged
        {
            Unsafe.SkipInit(out uint numBytes);
            fixed (void* pData = data)
            fixed (byte* pValidBytes = validBytes)
                LercAPICall(lerc_computeCompressedSize(
                    pData,
                    GetDataType<T>(),
                    nDepth, nCols,
                    nRows, nBands,
                    TranslateMaskSettings(nBands, masks),
                    validBytes.IsEmpty ? null : pValidBytes,
                    maxZErr,
                    &numBytes));
            return numBytes;
        }

        /// <summary>
        /// Encodes the provided ReadOnlySpan<T>. T can be byte, sbyte, short, ushort, int, uint, 
        /// float and double.
        /// </summary>
        /// <typeparam name="T">T can be byte, sbyte, short, ushort, int, uint, float and double.</typeparam>
        /// <param name="data">Data to compress, stored as vector.</param>
        /// <param name="nDepth">Number of multidimensional planes. Use 1 for common rasters.</param>
        /// <param name="nCols">Number of columns.</param>
        /// <param name="nRows">Number of rows.</param>
        /// <param name="nBands">Number of bands.</param>
        /// <param name="masks">Type of masking.</param>
        /// <param name="maxZErr">Maximal error of compression.</param>
        /// <param name="validBytes">Byte mask of valid pixels.</param>
        /// <param name="outBuffer">Preallocated buffer for output. If you leave it empty/default,
        /// if will be allocated & filled automatically.</param>
        /// <returns>Compressed bytes as Span<byte>. If outputBuffer is provided, no allocation occur.</returns>
        [SkipLocalsInit]
        public static unsafe Span<byte> Encode<T>([DisallowNull][NotNull] ReadOnlySpan<T> data,
                                                        int nDepth,
                                                        int nCols,
                                                        int nRows,
                                                        int nBands,
                                                        MaskSettings masks = MaskSettings.AllValid,
                                                        double maxZErr = 0d,
                                                        ReadOnlySpan<byte> validBytes = default,
                                                        Span<byte> outBuffer = default) where T : unmanaged
        {
            DataType dataType = GetDataType<T>();
            int nMasks = TranslateMaskSettings(nBands, masks);

            fixed (void* pData = data)
            fixed (byte* pValidBytes = validBytes)
            {
                if (outBuffer.IsEmpty)
                {
                    Unsafe.SkipInit(out uint numBytes);
                    LercAPICall(lerc_computeCompressedSize(
                                pData,
                                dataType,
                                nDepth, nCols,
                                nRows, nBands,
                                nMasks,
                                validBytes.IsEmpty ? null : pValidBytes,
                                maxZErr,
                                &numBytes));
                    outBuffer = GC.AllocateUninitializedArray<byte>((int)numBytes);
                }

                Unsafe.SkipInit(out uint nBytesWritten);
                fixed (byte* pOutBuffer = outBuffer)
                    LercAPICall(lerc_encode(
                        pData,
                        dataType,
                        nDepth, nCols,
                        nRows, nBands,
                        nMasks,
                        pValidBytes,
                        maxZErr,
                        pOutBuffer,
                        (uint)outBuffer.Length,
                        &nBytesWritten));

                int intBytesWritten = (int)nBytesWritten;
                if (intBytesWritten == outBuffer.Length)
                    return outBuffer;
                return outBuffer[..intBytesWritten];
            }
        }

        [SkipLocalsInit]
        public static void EncodeToFile<T>([DisallowNull][NotNull] string path,
                                             [DisallowNull][NotNull] ReadOnlySpan<T> data,
                                                        int nDepth,
                                                        int nCols,
                                                        int nRows,
                                                        int nBands,
                                                        MaskSettings masks = MaskSettings.AllValid,
                                                        double maxZErr = 0d,
                                                        ReadOnlySpan<byte> validBytes = default,
                                                        Span<byte> outBuffer = default) where T : unmanaged
        {
            using SafeFileHandle fileHandle = File.OpenHandle(path, FileMode.Create, FileAccess.Write, FileShare.None);
            RandomAccess.Write(fileHandle,
                Encode(data,
                       nDepth,
                       nCols,
                       nRows,
                       nBands,
                       masks,
                       maxZErr,
                       validBytes,
                       outBuffer),
                0L);
        }

        [SkipLocalsInit]
        public static unsafe LercBlobInfo GetLercBlobInfo([DisallowNull][NotNull] ReadOnlySpan<byte> lercBlob,
                                                          out DataRangeInfo dataRangeInfo)
        {
            Unsafe.SkipInit(out LercBlobInfo lercBlobInfo);
            fixed (byte* pLercBlob = lercBlob)
            fixed (DataRangeInfo* pDataRangeInfo = &dataRangeInfo)
                LercAPICall(lerc_getBlobInfo(pLercBlob,
                                             (uint)lercBlob.Length,
                                             &lercBlobInfo,
                                             pDataRangeInfo));
            return lercBlobInfo;
        }

        [SkipLocalsInit]
        public static unsafe Span<byte> Decode([DisallowNull][NotNull] ReadOnlySpan<byte> lercBlob,
                                                out LercBlobInfo lercBlobInfo,
                                                out DataRangeInfo dataRangeInfo,
                                                ref Span<byte> validBytesBuffer,
                                                Span<byte> outBuffer = default)
        {
            fixed (byte* pLercBlob = lercBlob)
            {
                fixed (LercBlobInfo* pLercBlobInfo = &lercBlobInfo)
                fixed (DataRangeInfo* pDataRangeInfo = &dataRangeInfo)
                    LercAPICall(lerc_getBlobInfo(pLercBlob,
                                                 (uint)lercBlob.Length,
                                                 pLercBlobInfo,
                                                 pDataRangeInfo));

                if (outBuffer.IsEmpty)
                    outBuffer = GC.AllocateUninitializedArray<byte>((int)lercBlobInfo.OutputBlobSize);

                if (lercBlobInfo.NMasks is not 0 && validBytesBuffer.IsEmpty)
                    validBytesBuffer = GC.AllocateUninitializedArray<byte>((int)lercBlobInfo.ByteMaskSize);

                fixed (byte* pData = outBuffer)
                fixed (byte* pValidBytes = validBytesBuffer)
                    LercAPICall(lerc_decode(pLercBlob,
                                            (uint)lercBlob.Length,
                                            (int)lercBlobInfo.NMasks,
                                            validBytesBuffer.IsEmpty ? null : pValidBytes,
                                            (int)lercBlobInfo.NDepth,
                                            (int)lercBlobInfo.NCols,
                                            (int)lercBlobInfo.NRows,
                                            (int)lercBlobInfo.NBands,
                                            lercBlobInfo.DataType,
                                            pData));
                return outBuffer;
            }
        }

        [SkipLocalsInit]
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static DataType GetDataType<T>() where T : unmanaged
        {
            return typeof(T) == typeof(sbyte) ? DataType.SByte :
              typeof(T) == typeof(byte) ? DataType.Byte :
              typeof(T) == typeof(short) ? DataType.Short :
              typeof(T) == typeof(ushort) ? DataType.UShort :
              typeof(T) == typeof(int) ? DataType.Int :
              typeof(T) == typeof(uint) ? DataType.UInt :
              typeof(T) == typeof(float) ? DataType.Float :
              typeof(T) == typeof(double) ? DataType.Double :
              ThrowUnsupportedDataType();

            [DoesNotReturn]
            static DataType ThrowUnsupportedDataType() =>
            throw new Exception("Unsupported data type.");
        }

        [SkipLocalsInit]
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static void LercAPICall(ErrorCode errorCode)
        {
            if (errorCode is not ErrorCode.Ok)
            {
                ThrowLercError(errorCode);

                [DoesNotReturn]
                [SkipLocalsInit]
                static void ThrowLercError(ErrorCode errorCode) =>
                throw new Exception(errorCode switch
                {
                    ErrorCode.Failed => "General function failure.",
                    ErrorCode.WrongParam => "Wrong parameter",
                    ErrorCode.BufferTooSmall => "Too small buffer provided.",
                    ErrorCode.NaN => "Unable to process NaN.",
                    ErrorCode.HasNoData => "There is no data.",
                    _ => $"Unknown error. Code: {errorCode}.",
                });
            }
        }

        [SkipLocalsInit]
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        private static int TranslateMaskSettings(int nBands, MaskSettings masks) => masks switch
        {
            MaskSettings.AllValid => 0,
            MaskSettings.SameMaskForAllBands => 1,
            MaskSettings.UniqueMaskForEveryBand => nBands,
            _ => throw new Exception("Invalid mask type.")
        };

        #region Native interop
        [DllImport(LercDLL, BestFitMapping = false, CallingConvention = CallingConvention.StdCall,
               EntryPoint = nameof(lerc_computeCompressedSize), ExactSpelling = true,
               PreserveSig = true, SetLastError = false)]
        private static extern unsafe ErrorCode lerc_computeCompressedSize(
                                          [DisallowNull][NotNull] void* pData,       // raw image data, row by row, band by band
                                          DataType dataType, // char = 0, uchar = 1, short = 2, ushort = 3, int = 4, uint = 5, float = 6, double = 7
                                          int nDepth,        // number of values per pixel (e.g., 3 for RGB, data is stored as [RGB, RGB, ...])
                                          int nCols,         // number of columns
                                          int nRows,         // number of rows
                                          int nBands,        // number of bands (e.g., 3 for [RRRR ..., GGGG ..., BBBB ...])
                                          int nMasks,        // 0 - all valid, 1 - same mask for all bands, nBands - masks can differ between bands
                                          byte* pValidBytes, // nullptr if all pixels are valid; otherwise 1 byte per pixel (1 = valid, 0 = invalid)
                                          double maxZErr,    // max coding error per pixel, defines the precision
                                          [DisallowNull][NotNull] uint* numBytes);   // size of outgoing Lerc blob

        [DllImport(LercDLL, BestFitMapping = false, CallingConvention = CallingConvention.StdCall,
           EntryPoint = nameof(lerc_encode), ExactSpelling = true,
           PreserveSig = true, SetLastError = false)]
        private static extern unsafe ErrorCode lerc_encode(
                                          [DisallowNull][NotNull] void* pData, // raw image data, row by row, band by band
                                          DataType dataType,                   // char = 0, uchar = 1, short = 2, ushort = 3, int = 4, uint = 5, float = 6, double = 7
                                          int nDepth,                          // number of values per pixel (e.g., 3 for RGB, data is stored as [RGB, RGB, ...])
                                          int nCols,                           // number of columns
                                          int nRows,                           // number of rows
                                          int nBands,                          // number of bands (e.g., 3 for [RRRR ..., GGGG ..., BBBB ...])
                                          int nMasks,                          // 0 - all valid, 1 - same mask for all bands, nBands - masks can differ between bands
                                          byte* pValidBytes,                   // nullptr if all pixels are valid; otherwise 1 byte per pixel (1 = valid, 0 = invalid)
                                          double maxZErr,                      // max coding error per pixel, defines the precision
                                          [DisallowNull][NotNull] byte* pOutBuffer, // buffer to write to, function fails if buffer too small
                                          uint outBufferSize,                  // size of output buffer
                                          [DisallowNull][NotNull] uint* nBytesWritten); // number of bytes written to output buffer

        [DllImport(LercDLL, BestFitMapping = false, CallingConvention = CallingConvention.StdCall,
            EntryPoint = nameof(lerc_getBlobInfo), ExactSpelling = true,
            PreserveSig = true, SetLastError = false)]
        private static extern unsafe ErrorCode lerc_getBlobInfo(
                                           [DisallowNull][NotNull] byte* pLercBlob, // Lerc blob to decode
                                           uint blobSize, // blob size in bytes
                                           [DisallowNull][NotNull] LercBlobInfo* info, // info array with all info needed to allocate the outgoing arrays for calling decode
                                           [DisallowNull][NotNull] DataRangeInfo* dataRange, // quick access to overall data range [zMin, zMax] without having to decode the data
                                           [ConstantExpected(Max = LercBlobInfo.Count, Min = LercBlobInfo.Count)] int infoFieldCount = LercBlobInfo.Count, // number of elements of infoArray
                                           [ConstantExpected(Max = DataRangeInfo.Count, Min = DataRangeInfo.Count)] int dataRangeFieldCount = DataRangeInfo.Count);           // number of elements of dataRangeArray

        [DllImport(LercDLL, BestFitMapping = false, CallingConvention = CallingConvention.StdCall,
    EntryPoint = nameof(lerc_decode), ExactSpelling = true,
    PreserveSig = true, SetLastError = false)]
        private static extern unsafe ErrorCode lerc_decode(
                                   [DisallowNull][NotNull] byte* pLercBlob, // Lerc blob to decode
                                   uint blobSize,             // blob size in bytes
                                   int nMasks,                // 0, 1, or nBands; return as many masks in the next array
                                   byte* pValidBytes,         // gets filled if not nullptr, even if all valid
                                   int nDepth,                // number of values per pixel (e.g., 3 for RGB, data is stored as [RGB, RGB, ...])
                                   int nCols,                 // number of columns
                                   int nRows,                 // number of rows
                                   int nBands,                // number of bands (e.g., 3 for [RRRR ..., GGGG ..., BBBB ...])
                                   DataType dataType,         // char = 0, uchar = 1, short = 2, ushort = 3, int = 4, uint = 5, float = 6, double = 7
                                   [DisallowNull][NotNull] void* pData); // outgoing data array

        #endregion Native interop
    };
}
