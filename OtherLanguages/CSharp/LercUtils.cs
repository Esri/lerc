// LercUtils.cs by Ákos Halmai @ University of Pécs
//
// Managed wrapper for the native Lerc library available at http://github.com/Esri/lerc/.
//
// Before using any Lerc operation, initialize this wrapper with the fully
// qualified path of lerc.dll:
//
//     LercUtils.Initialize(@"C:\NativeLibraries\Lerc\lerc.dll");
//
// Initialization is mandatory once per process execution. Relative paths are
// rejected, and normal DLL search-path fallback is intentionally disabled.
// Calling a Lerc operation before initialization throws
// InvalidOperationException.
//
// The selected DLL must exist, match the current process architecture, and
// export all native functions required by this wrapper. The initialized DLL
// cannot be replaced or changed while the application is running.

// PUBLIC OPERATIONS
//
// ComputeCompressedSize:
//   Calculates the minimum output-buffer size required for compression.
//   Call this before Encode when the required buffer size is unknown.
//
// Encode:
//   Compresses the supplied raster data into a Lerc blob. The output buffer
//   must be large enough; use ComputeCompressedSize to determine its size.
//
// GetLercInfo:
//   Reads dimensions, data type, mask count, and value-range information from
//   a Lerc blob without fully decoding it.
//
// Decode:
//   Decompresses a Lerc blob into the supplied data buffer. The destination
//   element type and size must match the information returned by GetLercInfo.
//
// All dimensions and buffers must describe the same raster layout.
// Native-pointer overloads are intended for advanced, unsafe use.

using System.ComponentModel;
using System.Diagnostics.CodeAnalysis;
using System.Reflection;
using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Lerc;

public enum LercDataType : uint
{
    SByte,
    Byte,
    Int16,
    UInt16,
    Int32,
    UInt32,
    Single,
    Double
}

[StructLayout(LayoutKind.Sequential, Pack = 1, Size = Size)]
[SkipLocalsInit]
public readonly record struct LercInfo(
    uint Version,
    LercDataType DataType,
    uint Depth,
    uint ColumnCount,
    uint RowCount,
    uint BandCount,
    uint ValidPixelCount,
    uint BlobSize,
    uint MaskCount,
    uint DimensionCount,
    uint UsesNoDataValue)
{
    internal const uint PropertyCount = 11;
    internal const int Size = (int)PropertyCount * sizeof(uint);
}

[StructLayout(LayoutKind.Sequential, Pack = 1, Size = Size)]
[SkipLocalsInit]
public readonly record struct RangeInfo(
    double MinimumValue,
    double MaximumValue,
    double MaximumErrorUsed)
{
    internal const uint PropertyCount = 3;
    internal const int Size = (int)PropertyCount * sizeof(double);
}

[SkipLocalsInit]
public static partial class LercUtils
{
    #region Initialization   
    private const string LercLibraryName = "COM1"; // Intentionally invalid.

    private static readonly Lock InitializationLock = new();

    private static string? _libraryPath;
    private static nint _libraryHandle;

    public static bool IsInitialized =>
    Volatile.Read(ref _libraryHandle) != nint.Zero;



    [ModuleInitializer]
    internal static void RegisterNativeLibraryResolver()
    {
        NativeLibrary.SetDllImportResolver(
        typeof(LercUtils).Assembly,
        ResolveNativeLibrary);
    }

    public static void Initialize(string fullyQualifiedLibraryPath)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(fullyQualifiedLibraryPath);
        if (!Path.IsPathFullyQualified(fullyQualifiedLibraryPath))
        { throw new ArgumentException("Use fully qualified path to the Lerc binary."); }
        string fullPath = Path.GetFullPath(fullyQualifiedLibraryPath); // Defensive.

        if (!File.Exists(fullPath))
        {
            throw new FileNotFoundException(
            "The Lerc native library was not found.",
            fullPath);
        }

        lock (InitializationLock)
        {
            if (_libraryHandle != nint.Zero)
            {
                if (!string.Equals(
                _libraryPath,
                fullPath,
                StringComparison.OrdinalIgnoreCase))
                {
                    throw new InvalidOperationException(
                    "Lerc has already been initialized from " +
                    $"'{_libraryPath}'.");
                }

                return;
            }

            nint handle = NativeLibrary.Load(fullPath);

            try
            {
                ValidateRequiredExports(handle);
            }
            catch
            {
                NativeLibrary.Free(handle);
                throw;
            }

            _libraryPath = fullPath;

            // Publish the handle only after all validation has succeeded.
            Volatile.Write(ref _libraryHandle, handle);
        }
    }

    private static nint ResolveNativeLibrary(
    string libraryName,
    Assembly assembly,
    DllImportSearchPath? searchPath)
    {
        if (!string.Equals(
        libraryName,
        LercLibraryName,
        StringComparison.Ordinal))
        {
            return nint.Zero;
        }

        nint handle = Volatile.Read(ref _libraryHandle);

        if (handle == nint.Zero)
        {
            throw new InvalidOperationException(
            "Lerc has not been initialized. Call " +
            "LercUtils.Initialize(pathToLercDll) before using " +
            "any Lerc operation.");
        }

        return handle;
    }

    private static void EnsureInitialized()
    {
        if (Volatile.Read(ref _libraryHandle) == nint.Zero)
        {
            throw new InvalidOperationException(
            "Lerc has not been initialized. Call " +
            "LercUtils.Initialize(pathToLercDll) before using " +
            "any Lerc operation.");
        }
    }

    private static void ValidateRequiredExports(nint handle)
    {
        string[] requiredExports =
        [
        "lerc_computeCompressedSize",
        "lerc_encode",
        "lerc_getBlobInfo",
        "lerc_decode"
        ];

        foreach (string exportName in requiredExports)
        {
            if (!NativeLibrary.TryGetExport(
            handle,
            exportName,
            out _))
            {
                throw new EntryPointNotFoundException(
                "The selected DLL is not a compatible Lerc library. " +
                $"The required export '{exportName}' is missing.");
            }
        }
    }
    #endregion

    #region Constants

    private const ulong MaximumBufferSizeInBytes = 4uL * 1024uL * 1024uL * 1024uL; // 4 GiB
    private const uint MinimumBlobSizeInBytes = 32u; // Not a real check, just to avoid 100% erroneous inputs.

    #endregion

    #region Types

    private enum LercErrorCode
    {
        Ok,
        Failed,
        WrongParameter,
        BufferTooSmall,
        NaN,
        HasNoData,
        DimensionsTooLarge
    }

    #endregion

    #region Native library imports

    [LibraryImport(LercLibraryName, EntryPoint = "lerc_computeCompressedSize")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    private static unsafe partial LercErrorCode NativeComputeCompressedSize(
        void* data,
        LercDataType dataType,
        uint depth,
        uint columnCount,
        uint rowCount,
        uint bandCount,
        uint maskCount,
        byte* validBytes,
        double maximumError,
        uint* byteCount);

    [LibraryImport(LercLibraryName, EntryPoint = "lerc_encode")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    private static unsafe partial LercErrorCode NativeEncode(
        void* data,
        LercDataType dataType,
        uint depth,
        uint columnCount,
        uint rowCount,
        uint bandCount,
        uint maskCount,
        byte* validBytes,
        double maximumError,
        byte* outputBuffer,
        uint outputBufferSize,
        uint* bytesWritten);

    [LibraryImport(LercLibraryName, EntryPoint = "lerc_getBlobInfo")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    private static unsafe partial LercErrorCode NativeGetBlobInfo(
        byte* lercBlob,
        uint blobSize,
        LercInfo* lercInfo,
        RangeInfo* rangeInfo,
        [ConstantExpected(Min = LercInfo.PropertyCount, Max = LercInfo.PropertyCount)]
        uint lercInfoSize = LercInfo.PropertyCount,
        [ConstantExpected(Min = RangeInfo.PropertyCount, Max = RangeInfo.PropertyCount)]
        uint rangeInfoSize = RangeInfo.PropertyCount);

    [LibraryImport(LercLibraryName, EntryPoint = "lerc_decode")]
    [UnmanagedCallConv(CallConvs = [typeof(CallConvCdecl)])]
    private static unsafe partial LercErrorCode NativeDecode(
        byte* lercBlob,
        uint blobSize,
        uint maskCount,
        byte* validBytes,
        uint depth,
        uint columnCount,
        uint rowCount,
        uint bandCount,
        LercDataType dataType,
        void* data);

    #endregion

    #region LercDataType extensions

    extension(LercDataType dataType)
    {
        private ulong SizeInBytes => dataType switch
        {
            LercDataType.SByte or LercDataType.Byte => 1UL,
            LercDataType.Int16 or LercDataType.UInt16 => 2UL,
            LercDataType.Int32 or LercDataType.UInt32 or LercDataType.Single => 4UL,
            LercDataType.Double => 8UL,
            _ => ThrowUndefinedEnumValueException(
                nameof(dataType),
                (int)dataType,
                typeof(LercDataType))
        };

        private void ThrowIfUndefined(
            [CallerArgumentExpression(nameof(dataType))]
            string? argumentName = null)
        {
            Type enumType = typeof(LercDataType);

            if (!Enum.IsDefined(enumType, dataType))
            {
                ThrowUndefinedEnumValueException(argumentName, (int)dataType, enumType);
            }
        }
    }

    #endregion

    #region Type helpers

    private static LercDataType GetLercDataType<T>()
        where T : unmanaged
    {
        return typeof(T) switch
        {
            Type type when type == typeof(sbyte) => LercDataType.SByte,
            Type type when type == typeof(byte) => LercDataType.Byte,
            Type type when type == typeof(short) => LercDataType.Int16,
            Type type when type == typeof(ushort) => LercDataType.UInt16,
            Type type when type == typeof(int) => LercDataType.Int32,
            Type type when type == typeof(uint) => LercDataType.UInt32,
            Type type when type == typeof(float) => LercDataType.Single,
            Type type when type == typeof(double) => LercDataType.Double,
            _ => ThrowUnsupportedDataTypeException()
        };
    }

    #endregion

    #region Validation

    private static unsafe void ValidateEncodeArguments(
        void* data,
        LercDataType dataType,
        uint depth,
        uint columnCount,
        uint rowCount,
        uint bandCount,
        uint maskCount,
        double maximumError,
        uint* outputCount)
    {
        ArgumentNullException.ThrowIfNull(data);
        dataType.ThrowIfUndefined();
        ArgumentOutOfRangeException.ThrowIfZero(depth);
        ArgumentOutOfRangeException.ThrowIfZero(columnCount);
        ArgumentOutOfRangeException.ThrowIfZero(rowCount);
        ArgumentOutOfRangeException.ThrowIfZero(bandCount);
        ArgumentNullException.ThrowIfNull(outputCount);

        ValidateMaskCount(bandCount, maskCount);

        if (dataType.SizeInBytes * depth * columnCount * rowCount * bandCount >
            MaximumBufferSizeInBytes)
        {
            ThrowRasterSizeExceededException();
        }

        if (!double.IsFinite(maximumError) || maximumError < 0d)
        {
            throw new ArgumentOutOfRangeException(nameof(maximumError));
        }
    }

    private static unsafe void ValidateDecodeArguments(
        void* lercBlob,
        uint blobSize,
        LercDataType dataType,
        uint depth,
        uint columnCount,
        uint rowCount,
        uint bandCount,
        uint maskCount,
        void* data)
    {
        ArgumentNullException.ThrowIfNull(lercBlob);
        ArgumentOutOfRangeException.ThrowIfLessThan(blobSize, MinimumBlobSizeInBytes);
        dataType.ThrowIfUndefined();
        ArgumentOutOfRangeException.ThrowIfZero(depth);
        ArgumentOutOfRangeException.ThrowIfZero(columnCount);
        ArgumentOutOfRangeException.ThrowIfZero(rowCount);
        ArgumentOutOfRangeException.ThrowIfZero(bandCount);
        ArgumentNullException.ThrowIfNull(data);

        if (dataType.SizeInBytes * depth * columnCount * rowCount * bandCount >
            MaximumBufferSizeInBytes)
        {
            ThrowRasterSizeExceededException();
        }

        ValidateMaskCount(bandCount, maskCount);
    }

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static void ValidateMaskCount(uint bandCount, uint maskCount)
    {
        if (maskCount > 1 && maskCount != bandCount)
        {
            ThrowInvalidMaskCountException();
        }
    }

    #endregion

    #region Native calls

    public static unsafe void ComputeCompressedSize(
        void* data,
        LercDataType dataType,
        uint depth,
        uint columnCount,
        uint rowCount,
        uint bandCount,
        uint maskCount,
        byte* validBytes,
        double maximumError,
        uint* byteCount)
    {
        EnsureInitialized();
        ValidateEncodeArguments(
            data,
            dataType,
            depth,
            columnCount,
            rowCount,
            bandCount,
            maskCount,
            maximumError,
            byteCount);

        ThrowIfLercError(NativeComputeCompressedSize(
            data,
            dataType,
            depth,
            columnCount,
            rowCount,
            bandCount,
            maskCount,
            validBytes,
            maximumError,
            byteCount));
    }

    public static unsafe void Encode(
        void* data,
        LercDataType dataType,
        uint depth,
        uint columnCount,
        uint rowCount,
        uint bandCount,
        uint maskCount,
        byte* validBytes,
        double maximumError,
        byte* outputBuffer,
        uint outputBufferSize,
        uint* bytesWritten)
    {
        EnsureInitialized();
        ValidateEncodeArguments(
            data,
            dataType,
            depth,
            columnCount,
            rowCount,
            bandCount,
            maskCount,
            maximumError,
            bytesWritten);
        ArgumentNullException.ThrowIfNull(outputBuffer);
        ArgumentOutOfRangeException.ThrowIfLessThan(
            outputBufferSize,
            MinimumBlobSizeInBytes);

        ThrowIfLercError(NativeEncode(
            data,
            dataType,
            depth,
            columnCount,
            rowCount,
            bandCount,
            maskCount,
            validBytes,
            maximumError,
            outputBuffer,
            outputBufferSize,
            bytesWritten));
    }

    public static unsafe void GetLercInfo(
        byte* lercBlob,
        uint blobSize,
        LercInfo* lercInfo,
        RangeInfo* rangeInfo)
    {
        EnsureInitialized();
        ArgumentNullException.ThrowIfNull(lercBlob);
        ArgumentOutOfRangeException.ThrowIfLessThan(blobSize, MinimumBlobSizeInBytes);
        ArgumentNullException.ThrowIfNull(lercInfo);
        ArgumentNullException.ThrowIfNull(rangeInfo);

        ThrowIfLercError(NativeGetBlobInfo(lercBlob, blobSize, lercInfo, rangeInfo));
    }

    public static unsafe void Decode(
        byte* lercBlob,
        uint blobSize,
        uint maskCount,
        byte* validBytes,
        uint depth,
        uint columnCount,
        uint rowCount,
        uint bandCount,
        LercDataType dataType,
        void* data)
    {
        EnsureInitialized();
        ValidateDecodeArguments(
            lercBlob,
            blobSize,
            dataType,
            depth,
            columnCount,
            rowCount,
            bandCount,
            maskCount,
            data);

        ThrowIfLercError(NativeDecode(
            lercBlob,
            blobSize,
            maskCount,
            validBytes,
            depth,
            columnCount,
            rowCount,
            bandCount,
            dataType,
            data));
    }

    #endregion

    #region Managed calls

    public static unsafe uint ComputeCompressedSize<T>(
        ReadOnlySpan<T> data,
        uint depth,
        uint columnCount,
        uint rowCount,
        uint bandCount,
        uint maskCount,
        ReadOnlySpan<byte> validBytes,
        double maximumError)
        where T : unmanaged
    {
        LercDataType dataType = GetLercDataType<T>();
        ArgumentOutOfRangeException.ThrowIfLessThan((uint)data.Length, depth * columnCount * rowCount * bandCount);
        Unsafe.SkipInit(out uint byteCount);

        fixed (void* dataPointer = data)
        fixed (byte* validBytesPointer = validBytes)
        {
            ComputeCompressedSize(
                dataPointer,
                dataType,
                depth,
                columnCount,
                rowCount,
                bandCount,
                maskCount,
                validBytesPointer,
                maximumError,
                &byteCount);
        }

        return byteCount;
    }

    public static unsafe void Encode<T>(
        ReadOnlySpan<T> data,
        uint depth,
        uint columnCount,
        uint rowCount,
        uint bandCount,
        uint maskCount,
        ReadOnlySpan<byte> validBytes,
        double maximumError,
        Span<byte> outputBuffer,
        out uint bytesWritten)
        where T : unmanaged
    {
        LercDataType dataType = GetLercDataType<T>();

        ArgumentOutOfRangeException.ThrowIfLessThan((uint)data.Length, depth * columnCount * rowCount * bandCount);

        fixed (void* dataPointer = data)
        fixed (byte* validBytesPointer = validBytes)
        fixed (byte* outputBufferPointer = outputBuffer)
        fixed (uint* bytesWrittenPointer = &bytesWritten)
        {
            Encode(
                dataPointer,
                dataType,
                depth,
                columnCount,
                rowCount,
                bandCount,
                maskCount,
                validBytesPointer,
                maximumError,
                outputBufferPointer,
                (uint)outputBuffer.Length,
                bytesWrittenPointer);
        }
    }

    public static unsafe LercInfo GetLercInfo(
        ReadOnlySpan<byte> lercBlob,
        out RangeInfo rangeInfo)
    {
        Unsafe.SkipInit(out LercInfo lercInfo);

        fixed (byte* lercBlobPointer = lercBlob)
        fixed (RangeInfo* rangeInfoPointer = &rangeInfo)
        {
            GetLercInfo(
                lercBlobPointer,
                (uint)lercBlob.Length,
                &lercInfo,
                rangeInfoPointer);
        }

        return lercInfo;
    }

    public static unsafe void Decode<T>(
        ReadOnlySpan<byte> lercBlob,
        uint maskCount,
        uint depth,
        uint columnCount,
        uint rowCount,
        uint bandCount,
        Span<T> data,
        Span<byte> validBytes = default)
        where T : unmanaged
    {
        LercDataType dataType = GetLercDataType<T>();
        ArgumentOutOfRangeException.ThrowIfLessThan((uint)data.Length, depth * columnCount * rowCount * bandCount);
        ulong requiredValidByteCount =
        (ulong)columnCount * rowCount * maskCount;

        if (!validBytes.IsEmpty &&
        (ulong)validBytes.Length < requiredValidByteCount)
        {
            ThrowInvalidValidBytesBufferSizeException();
        }

        fixed (byte* lercBlobPointer = lercBlob)
        fixed (byte* validBytesPointer = validBytes)
        fixed (void* dataPointer = data)
        {
            Decode(
                lercBlobPointer,
                (uint)lercBlob.Length,
                maskCount,
                validBytesPointer,
                depth,
                columnCount,
                rowCount,
                bandCount,
                dataType,
                dataPointer);
        }
    }

    public static void Decode<T>(
        ReadOnlySpan<byte> lercBlob,
        in LercInfo lercInfo,
        Span<T> data,
        Span<byte> validBytes = default)
        where T : unmanaged
    {
        if (GetLercDataType<T>() != lercInfo.DataType)
        {
            ThrowDataTypeMismatchException();
        }

        Decode(
            lercBlob,
            lercInfo.MaskCount,
            lercInfo.Depth,
            lercInfo.ColumnCount,
            lercInfo.RowCount,
            lercInfo.BandCount,
            data,
            validBytes);
    }

    #endregion

    #region Exception helpers

    [MethodImpl(MethodImplOptions.AggressiveInlining)]
    private static void ThrowIfLercError(LercErrorCode errorCode)
    {
        if (errorCode != LercErrorCode.Ok)
        {
            ThrowLercException(errorCode);
        }
    }

    [DoesNotReturn]
    private static ulong ThrowUndefinedEnumValueException(
        string? argumentName,
        int invalidValue,
        Type enumType) =>
        throw new InvalidEnumArgumentException(argumentName, invalidValue, enumType);

    [DoesNotReturn]
    private static LercDataType ThrowUnsupportedDataTypeException() =>
        throw new NotSupportedException("The data type is not supported.");

    [DoesNotReturn]
    private static void ThrowRasterSizeExceededException() =>
        throw new OverflowException("The maximum raster size was exceeded.");

    [DoesNotReturn]
    private static void ThrowMaximumErrorOutOfRangeException(string? argumentName) =>
        throw new ArgumentOutOfRangeException(argumentName);

    [DoesNotReturn]
    private static void ThrowInvalidMaskCountException() =>
        throw new ArgumentException(
            "The mask count must be zero, one, or equal to the band count.",
            "maskCount");

    [DoesNotReturn]
    private static void ThrowInvalidValidBytesBufferSizeException() =>
        throw new ArgumentException(
            "The valid-bytes buffer is too small.",
            "validBytes");

    [DoesNotReturn]
    private static void ThrowDataTypeMismatchException() =>
        throw new ArgumentException(
            "The Lerc data type does not match the destination span element type.",
            "data");

    [DoesNotReturn]
    private static void ThrowLercException(LercErrorCode errorCode) =>
        throw new InvalidOperationException(
            $"The underlying Lerc library returned error '{errorCode}'.");

    #endregion
}
