using Microsoft.Win32.SafeHandles;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace LercCS
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
    public static class LercCS
    {
        public const string LercDLL = @"Lerc.dll";

        public static bool IsLercDLLAvailable
        {
            [SkipLocalsInit]
            [MethodImpl(MethodImplOptions.AggressiveInlining)]
            get => File.Exists(LercDLL);
        }

        [MethodImpl(MethodImplOptions.AggressiveInlining), SkipLocalsInit]
        public static unsafe uint ComputeCompressedSize<T>(ReadOnlySpan<T> data,
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
            fixed (byte* pValidBytes = validBytes.IsEmpty ? null : validBytes)
                LercAPICall(lerc_computeCompressedSize(
                    pData,
                    GetDataType<T>(),
                    nDepth, nCols,
                    nRows, nBands,
                    TranslateMaskSettings(nBands, masks),
                    pValidBytes,
                    maxZErr,
                    &numBytes));
            return numBytes;
        }

        [SkipLocalsInit]
        public static unsafe Span<byte> Encode<T>(ReadOnlySpan<T> data,
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
            fixed (byte* pValidBytes = validBytes.IsEmpty ? null : validBytes)
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
                                pValidBytes,
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

        internal static void EncodeToFile<T>(string path, ReadOnlySpan<T> data,
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
          [DisallowNull][NotNull] void* pData,                 // raw image data, row by row, band by band
          DataType dataType,             // char = 0, uchar = 1, short = 2, ushort = 3, int = 4, uint = 5, float = 6, double = 7
          int nDepth,                        // number of values per pixel (e.g., 3 for RGB, data is stored as [RGB, RGB, ...])
          int nCols,                         // number of columns
          int nRows,                         // number of rows
          int nBands,                        // number of bands (e.g., 3 for [RRRR ..., GGGG ..., BBBB ...])
          int nMasks,                        // 0 - all valid, 1 - same mask for all bands, nBands - masks can differ between bands
          byte* pValidBytes,  // nullptr if all pixels are valid; otherwise 1 byte per pixel (1 = valid, 0 = invalid)
          double maxZErr,                    // max coding error per pixel, defines the precision
          [DisallowNull][NotNull] byte* pOutBuffer,         // buffer to write to, function fails if buffer too small
          uint outBufferSize,        // size of output buffer
          [DisallowNull][NotNull] uint* nBytesWritten);      // number of bytes written to output buffer
    };
}
