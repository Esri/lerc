// Alternative implementation for Lerc Encoding/Decoding in C#.
// By Ákos Halmai 2024.

using System;
using System.IO;
using System.Diagnostics.CodeAnalysis;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;


namespace LercNS;

public enum ErrorCode : int
{
    Ok,
    Failed,
    WrongParam,
    BufferTooSmall,
    NaN,
    HasNoData
};

public enum DataType : uint
{
    SByte,
    Byte,
    Short,
    UShort,
    Int,
    UInt,
    Float,
    Double
};

public enum MaskType : int
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

    private const int Size = Count * sizeof(uint);
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
    public readonly bool UsesNoDataValue
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        [SkipLocalsInit]
        get => UsesNoDataValueInternal is not 0u;
    }

    public readonly MaskType MaskType
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        [SkipLocalsInit]
        get => NMasks switch
        {
            0u => MaskType.AllValid,
            1u => MaskType.SameMaskForAllBands,
            _ => MaskType.UniqueMaskForEveryBand,
        };
    }

    public readonly uint DecodedRasterDataSize
    {
        [SkipLocalsInit]
        get
        {
            uint decodedRasterDataSize = NDepth * NCols * NRows * NBands * (DataType switch
            {
                DataType.Byte or DataType.SByte => sizeof(byte), // 1 byte
                DataType.Short or DataType.UShort => sizeof(short), // 2 bytes
                DataType.Int or DataType.UInt or DataType.Float => sizeof(int), // 4 bytes
                DataType.Double => sizeof(double), // 8 bytes
                _ => ThrowUnsupportedDataType()
            });

            return decodedRasterDataSize is not 0u ? decodedRasterDataSize : ThrowInvalidBlobSize();

            [DoesNotReturn]
            [SkipLocalsInit]
            static uint ThrowInvalidBlobSize() =>
            throw new Exception("Invalid blob size. The struct might be uninitialized.");

            [DoesNotReturn]
            [SkipLocalsInit]
            static uint ThrowUnsupportedDataType() =>
             throw new Exception("Unsupported data type.");
        }
    }

    public readonly uint PixelMasksLength
    {
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        [SkipLocalsInit]
        get
        {
            uint pixelMaskLength = NCols * NRows * NMasks;
            if (pixelMaskLength is 0u)
            {
                ThrowInvalidPixelMasksLength();

                [DoesNotReturn]
                [SkipLocalsInit]
                static void ThrowInvalidPixelMasksLength() =>
                throw new Exception("Invalid pixel mask size. The struct might be uninitialized.");
            }
            return pixelMaskLength;
        }
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    [SkipLocalsInit]
    public readonly void Deconstruct(out DataType dataType,
                                     out int nDepth,
                                     out int nCols,
                                     out int nRows,
                                     out int nBands,
                                     out MaskType maskType)
    {
        dataType = DataType;
        nDepth = (int)NDepth;
        nCols = (int)NCols;
        nRows = (int)NRows;
        nBands = (int)NBands;
        maskType = MaskType;
    }
};

[SkipLocalsInit]
[StructLayout(LayoutKind.Sequential, Pack = 1, Size = Size)]
public readonly struct DataRangeInfo
{
    public const int Count = 3;
    private const int Size = Count * sizeof(double);
    public readonly double ZMin { get; }
    public readonly double ZMax { get; }
    public readonly double MaxZErrorUsed { get; }
};
public static class LercCS
{
    [MethodImpl(MethodImplOptions.AggressiveInlining), SkipLocalsInit]
    public static unsafe uint ComputeEncodedSize<T>(ReadOnlySpan<T> rasterData,
                                                    int nDepth,
                                                    int nCols,
                                                    int nRows,
                                                    int nBands,
                                                    MaskType maskType = MaskType.AllValid,
                                                    double maxZErr = 0d,
                                                    ReadOnlySpan<byte> pixelMasks = default) where T : unmanaged
    {
        fixed (T* pData = rasterData)
        fixed (byte* pValidBytes = pixelMasks)
        {
            Unsafe.SkipInit(out uint numBytes);
            LercAPICall(lerc_computeCompressedSize(
                pData,
                GetLercDataType<T>(),
                nDepth, nCols,
                nRows, nBands,
                MaskTypeToNMask(nBands, maskType),
                pValidBytes,
                maxZErr,
                &numBytes));
            return numBytes;
        }
    }

    /// <summary>
    /// Encodes the provided ReadOnlySpan of T. T can be byte, sbyte, short, ushort, int, uint, 
    /// float and double.
    /// </summary>
    /// <typeparam name="T">T can be byte, sbyte, short, ushort, int, uint, float and double.</typeparam>
    /// <param name="rasterData">Data to compress, stored as vector.</param>
    /// <param name="nDepth">Number of values per pixel.</param>
    /// <param name="nCols">Number of columns.</param>
    /// <param name="nRows">Number of rows.</param>
    /// <param name="nBands">Number of bands.</param>
    /// <param name="maskType">Type of masking.</param>
    /// <param name="maxZErr">Maximal error of compression.</param>
    /// <param name="pixelMasks">Byte mask of valid pixels.</param>
    /// <param name="encodedLercBlobBuffer">Pre-allocated buffer for output. If you leave it empty/default or under-allocated
    /// it will be allocated and filled automatically.</param>
    /// <returns>Compressed bytes as Span of byte. If outputBuffer is provided, no allocation occur.</returns>
    [SkipLocalsInit]
    public static unsafe Span<byte> Encode<T>(ReadOnlySpan<T> rasterData,
                                                    int nDepth,
                                                    int nCols,
                                                    int nRows,
                                                    int nBands,
                                                    MaskType maskType = MaskType.AllValid,
                                                    double maxZErr = 0d,
                                                    ReadOnlySpan<byte> pixelMasks = default,
                                                    Span<byte> encodedLercBlobBuffer = default) where T : unmanaged
    {
        fixed (T* pData = rasterData)
        fixed (byte* pValidBytes = pixelMasks)
        {
            DataType dataType = GetLercDataType<T>();
            int nMasks = MaskTypeToNMask(nBands, maskType);

            const int headerSize = 93;
            int bufferLength = encodedLercBlobBuffer.Length;
            if (bufferLength < headerSize) // Header size. This is the absolute minimum.
            {
                LercAPICall(lerc_computeCompressedSize(
                            pData,
                            dataType,
                            nDepth, nCols,
                            nRows, nBands,
                            nMasks,
                            pValidBytes,
                            maxZErr,
                            (uint*)&bufferLength));

                if (bufferLength < headerSize || bufferLength > Array.MaxLength)
                {
                    ThrowUnableToAllocate();
                }

                encodedLercBlobBuffer = GC.AllocateUninitializedArray<byte>(bufferLength);

                [DoesNotReturn]
                [SkipLocalsInit]
                static void ThrowUnableToAllocate() =>
                    throw new Exception("Invalid size for memory allocation.");
            }

            fixed (byte* pOutBuffer = encodedLercBlobBuffer)
            {
                Unsafe.SkipInit(out uint nBytesWritten);
                LercAPICall(lerc_encode(
                    pData,
                    dataType,
                    nDepth, nCols,
                    nRows, nBands,
                    nMasks,
                    pValidBytes,
                    maxZErr,
                    pOutBuffer,
                    (uint)bufferLength,
                    &nBytesWritten));

                int intBytesWritten = (int)nBytesWritten;
                return intBytesWritten == bufferLength ?
                    encodedLercBlobBuffer : encodedLercBlobBuffer[..intBytesWritten];
            }
        }
    }

    [SkipLocalsInit]
    public static unsafe LercBlobInfo GetLercBlobInfo(ReadOnlySpan<byte> encodedLercBlob,
                                                      out DataRangeInfo dataRangeInfo)
    {
        fixed (byte* pLercBlob = encodedLercBlob)
        fixed (DataRangeInfo* pDataRangeInfo = &dataRangeInfo)
        {
            Unsafe.SkipInit(out LercBlobInfo lercBlobInfo);
            LercAPICall(lerc_getBlobInfo(pLercBlob,
                                         (uint)encodedLercBlob.Length,
                                         &lercBlobInfo,
                                         pDataRangeInfo));
            return lercBlobInfo;
        }
    }

    [SkipLocalsInit]
    public static unsafe void GetDataRanges(ReadOnlySpan<byte> encodedLercBlob,
                                                  int nDepth,
                                                  int nBands,
                                                  ref Span<double> MinValues,
                                                  ref Span<double> MaxValues)
    {
        int expectedLength = nDepth * nBands;

        if (MinValues.Length < expectedLength)
            MinValues = GC.AllocateUninitializedArray<double>(expectedLength);

        if (MaxValues.Length < expectedLength)
            MaxValues = GC.AllocateUninitializedArray<double>(expectedLength);

        fixed (double* pMins = MinValues)
        fixed (double* pMaxs = MaxValues)
        fixed (byte* pLercBlob = encodedLercBlob)
            LercAPICall(lerc_getDataRanges(pLercBlob,
                (uint)encodedLercBlob.Length,
                nDepth, nBands,
                pMins, pMaxs));
    }

    [SkipLocalsInit]
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    public static unsafe Span<byte> Decode(ReadOnlySpan<byte> elcodedLercBlob,
                                            out LercBlobInfo lercBlobInfo,
                                            out DataRangeInfo dataRangeInfo,
                                            ref Span<byte> pixelMasks,
                                            Span<byte> rasterDataBuffer = default) =>
        Decode(elcodedLercBlob,
               lercBlobInfo = GetLercBlobInfo(elcodedLercBlob, out dataRangeInfo),
               ref pixelMasks,
               rasterDataBuffer);

    [SkipLocalsInit]
    public static unsafe Span<byte> Decode(ReadOnlySpan<byte> encodedLercBlob,
                                     LercBlobInfo lercBlobInfo,
                                     ref Span<byte> pixelMasks,
                                     Span<byte> rasterDataBuffer = default)
    {

        int decodedRasterDataSize = (int)lercBlobInfo.DecodedRasterDataSize;

        if (rasterDataBuffer.Length < decodedRasterDataSize)
            rasterDataBuffer = GC.AllocateUninitializedArray<byte>(decodedRasterDataSize);

        if (lercBlobInfo.NMasks is not 0)
        {
            int pixelMaskSize = (int)lercBlobInfo.PixelMasksLength;
            if (pixelMasks.Length < pixelMaskSize)
                pixelMasks = GC.AllocateUninitializedArray<byte>(pixelMaskSize);
        }
        else
        {
            pixelMasks = default;
        }

        fixed (byte* pLercBlob = encodedLercBlob)
        fixed (byte* pData = rasterDataBuffer)
        fixed (byte* pValidBytes = pixelMasks)
        {
            LercAPICall(lerc_decode(pLercBlob,
                                    (uint)encodedLercBlob.Length,
                                    (int)lercBlobInfo.NMasks,
                                    pValidBytes,
                                    (int)lercBlobInfo.NDepth,
                                    (int)lercBlobInfo.NCols,
                                    (int)lercBlobInfo.NRows,
                                    (int)lercBlobInfo.NBands,
                                    lercBlobInfo.DataType,
                                    pData));

            return rasterDataBuffer.Length != decodedRasterDataSize ?
                rasterDataBuffer[..decodedRasterDataSize] : rasterDataBuffer;
        }
    }

    #region "Utility functions"

    [SkipLocalsInit]
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static DataType GetLercDataType<T>() where T : unmanaged
    {
        return typeof(T) == typeof(sbyte) ? DataType.SByte :
          typeof(T) == typeof(byte) ? DataType.Byte :
          typeof(T) == typeof(short) ? DataType.Short :
          typeof(T) == typeof(ushort) ? DataType.UShort :
          typeof(T) == typeof(int) ? DataType.Int :
          typeof(T) == typeof(uint) ? DataType.UInt :
          typeof(T) == typeof(float) ? DataType.Float :
          typeof(T) == typeof(double) ? DataType.Double :
          ThrowUnsupportedLercDataType();

        [DoesNotReturn]
        [SkipLocalsInit]
        static DataType ThrowUnsupportedLercDataType() =>
            throw new Exception("Unsupported data type.");
    }

    [SkipLocalsInit]
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static void LercAPICall(ErrorCode errorCode)
    {
        if (errorCode is not ErrorCode.Ok)
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

    [SkipLocalsInit]
    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static int MaskTypeToNMask(int nBands, MaskType maskType)
    {
        return maskType switch
        {
            MaskType.AllValid => 0,
            MaskType.SameMaskForAllBands => 1,
            MaskType.UniqueMaskForEveryBand => nBands,
            _ => ThrowInvalidMaskType()
        };

        [DoesNotReturn]
        [SkipLocalsInit]
        static int ThrowInvalidMaskType() =>
            throw new Exception("Invalid mask type.");
    }

    #endregion "Utility functions"

    #region "Native P/Invoke"
    private const string LercDLL = @"Lerc.dll";
    public static bool IsLercDLLAvailable
    {
        [SkipLocalsInit]
        [MethodImpl(MethodImplOptions.AggressiveInlining)]
        get => File.Exists(LercDLL);
    }

    // SYSLIB1054 warnings suppressed, because there is no marshaling involved, no marshaling code needed.
#pragma warning disable SYSLIB1054 // Use 'LibraryImportAttribute' instead of 'DllImportAttribute' to generate P/Invoke marshalling code at compile time

    [DllImport(LercDLL, BestFitMapping = false,
               CallingConvention = CallingConvention.StdCall,
               ExactSpelling = true)]
    private static extern unsafe ErrorCode lerc_computeCompressedSize(
                                      [DisallowNull][NotNull] void* pData,       // raw image data, row by row, band by band
                                      DataType dataType, // char = 0, uchar = 1, short = 2, ushort = 3, int = 4, uint = 5, float = 6, double = 7
                                      int nDepth,        // number of values per pixel (e.g., 3 for RGB, data is stored as [RGB, RGB, …])
                                      int nCols,         // number of columns
                                      int nRows,         // number of rows
                                      int nBands,        // number of bands (e.g., 3 for [RRRR …, GGGG …, BBBB …])
                                      int nMasks,        // 0 - all valid, 1 - same mask for all bands, nBands - masks can differ between bands
                                      byte* pValidBytes, // nullptr if all pixels are valid; otherwise 1 byte per pixel (1 = valid, 0 = invalid)
                                      double maxZErr,    // max coding error per pixel, defines the precision
                                      [DisallowNull][NotNull] uint* numBytes);   // size of outgoing Lerc blob

    [DllImport(LercDLL, BestFitMapping = false,
               CallingConvention = CallingConvention.StdCall,
               ExactSpelling = true)]
    private static extern unsafe ErrorCode lerc_encode(
                                      [DisallowNull][NotNull] void* pData, // raw image data, row by row, band by band
                                      DataType dataType,                   // char = 0, uchar = 1, short = 2, ushort = 3, int = 4, uint = 5, float = 6, double = 7
                                      int nDepth,                          // number of values per pixel (e.g., 3 for RGB, data is stored as [RGB, RGB, …])
                                      int nCols,                           // number of columns
                                      int nRows,                           // number of rows
                                      int nBands,                          // number of bands (e.g., 3 for [RRRR …, GGGG …, BBBB …])
                                      int nMasks,                          // 0 - all valid, 1 - same mask for all bands, nBands - masks can differ between bands
                                      byte* pValidBytes,                   // nullptr if all pixels are valid; otherwise 1 byte per pixel (1 = valid, 0 = invalid)
                                      double maxZErr,                      // max coding error per pixel, defines the precision
                                      [DisallowNull][NotNull] byte* pOutBuffer, // buffer to write to, function fails if buffer too small
                                      uint outBufferSize,                  // size of output buffer
                                      [DisallowNull][NotNull] uint* nBytesWritten); // number of bytes written to output buffer

    [DllImport(LercDLL, BestFitMapping = false,
               CallingConvention = CallingConvention.StdCall,
               ExactSpelling = true)]
    private static extern unsafe ErrorCode lerc_getBlobInfo(
                                       [DisallowNull][NotNull] byte* pLercBlob, // Lerc blob to decode
                                       uint blobSize, // blob size in bytes
                                       [DisallowNull][NotNull] LercBlobInfo* info, // info array with all info needed to allocate the outgoing arrays for calling decode
                                       [DisallowNull][NotNull] DataRangeInfo* dataRange, // quick access to overall data range [zMin, zMax] without having to decode the data
                                       [ConstantExpected(Max = LercBlobInfo.Count, Min = LercBlobInfo.Count)] int infoFieldCount = LercBlobInfo.Count, // number of elements of infoArray
                                       [ConstantExpected(Max = DataRangeInfo.Count, Min = DataRangeInfo.Count)] int dataRangeFieldCount = DataRangeInfo.Count);           // number of elements of dataRangeArray

    [DllImport(LercDLL, BestFitMapping = false,
       CallingConvention = CallingConvention.StdCall,
       ExactSpelling = true)]
    private static extern unsafe ErrorCode lerc_getDataRanges(
                       [DisallowNull][NotNull] byte* pLercBlob, // Lerc blob to decode
                       uint blobSize,             // blob size in bytes
                       int nDepth,                // number of values per pixel (e.g., 3 for RGB, data is stored as [RGB, RGB, …])
                       int nBands,                // number of bands (e.g., 3 for [RRRR …, GGGG …, BBBB …])
                       [DisallowNull][NotNull] double* pMins,  // outgoing minima per dimension and band 
                       [DisallowNull][NotNull] double* pMaxs); // outgoing maxima per dimension and band

    [DllImport(LercDLL, BestFitMapping = false,
               CallingConvention = CallingConvention.StdCall,
               ExactSpelling = true)]
    private static extern unsafe ErrorCode lerc_decode(
                               [DisallowNull][NotNull] byte* pLercBlob, // Lerc blob to decode
                               uint blobSize,             // blob size in bytes
                               int nMasks,                // 0, 1, or nBands; return as many masks in the next array
                               byte* pValidBytes,         // gets filled if not nullptr, even if all valid
                               int nDepth,                // number of values per pixel (e.g., 3 for RGB, data is stored as [RGB, RGB, …])
                               int nCols,                 // number of columns
                               int nRows,                 // number of rows
                               int nBands,                // number of bands (e.g., 3 for [RRRR …, GGGG …, BBBB …])
                               DataType dataType,         // char = 0, uchar = 1, short = 2, ushort = 3, int = 4, uint = 5, float = 6, double = 7
                               [DisallowNull][NotNull] void* pData); // outgoing data array

#pragma warning restore SYSLIB1054 // Use 'LibraryImportAttribute' instead of 'DllImportAttribute' to generate P/Invoke marshalling code at compile time
    #endregion "Native P/Invoke"
};
