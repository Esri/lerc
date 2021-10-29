import lercWasm, { LercFactory } from "./lerc-wasm";

type PixelTypedArray =
  | Int8Array
  | Uint8Array
  | Uint8ClampedArray
  | Int16Array
  | Uint16Array
  | Int32Array
  | Uint32Array
  | Float32Array
  | Float64Array;

type PixelTypedArrayCtor =
  | Int8ArrayConstructor
  | Uint8ArrayConstructor
  | Uint8ClampedArrayConstructor
  | Int16ArrayConstructor
  | Uint16ArrayConstructor
  | Int32ArrayConstructor
  | Uint32ArrayConstructor
  | Float32ArrayConstructor
  | Float64ArrayConstructor;

interface BandStats {
  minValue: number;
  maxValue: number;
  dimStats?: {
    // Note the type change compared to the JS version which used typed array that fits the data's pixel type
    minValues: Float64Array;
    maxValues: Float64Array;
  };
}

interface LercHeaderInfo {
  version: number;
  dimCount: number;
  width: number;
  height: number;
  validPixelCount: number;
  bandCount: number;
  blobSize: number;
  maskCount: number;
  dataType: number;
  minValue: number;
  maxValue: number;
  maxZerror: number;
  statistics: BandStats[];
}

type LercPixelType =
  | "S8"
  | "U8"
  | "S16"
  | "U16"
  | "S32"
  | "U32"
  | "F32"
  | "F64";

interface LercData {
  width: number;
  height: number;
  bandCount: number;
  pixelType: LercPixelType;
  dimCount: number;
  statistics: BandStats[];
  pixels: PixelTypedArray[];
  mask: Uint8Array;
  bandMasks?: Uint8Array[];
}

interface PixelTypeInfo {
  pixelType: LercPixelType;
  size: 1 | 2 | 4 | 8;
  ctor: PixelTypedArrayCtor;
  range: [number, number];
}

const pixelTypeInfoMap: PixelTypeInfo[] = [
  {
    pixelType: "S8",
    size: 1,
    ctor: Int8Array,
    range: [-128, 128]
  },
  {
    pixelType: "U8",
    size: 1,
    ctor: Uint8Array,
    range: [0, 255]
  },
  {
    pixelType: "S16",
    size: 2,
    ctor: Int16Array,
    range: [-32768, 32767]
  },
  {
    pixelType: "U16",
    size: 2,
    ctor: Uint16Array,
    range: [0, 65536]
  },
  {
    pixelType: "S32",
    size: 4,
    ctor: Int32Array,
    range: [-2147483648, 2147483647]
  },
  {
    pixelType: "U32",
    size: 4,
    ctor: Uint32Array,
    range: [0, 4294967296]
  },
  {
    pixelType: "F32",
    size: 4,
    ctor: Float32Array,
    range: [-3.4027999387901484e38, 3.4027999387901484e38]
  },
  {
    pixelType: "F64",
    size: 8,
    ctor: Float64Array,
    range: [-1.7976931348623157e308, 1.7976931348623157e308]
  }
];

let loadPromise: Promise<void> = null;
let loaded = false;
export function load(
  options: {
    locateFile?: (wasmFileName?: string, scriptDir?: string) => string;
  } = {}
): Promise<void> {
  if (loadPromise) {
    return loadPromise;
  }
  const locateFile =
    options.locateFile ||
    ((wasmFileName: string, scriptDir: string) =>
      `${scriptDir}${wasmFileName}`);
  loadPromise = lercWasm({ locateFile }).then((lercFactory) =>
    lercFactory.ready.then(() => {
      initLercLib(lercFactory);
      loaded = true;
    })
  );
  return loadPromise;
}

export function isLoaded(): boolean {
  return loaded;
}

const lercLib: {
  getBlobInfo: (_blob: Uint8Array) => LercHeaderInfo;
  decode: (
    _blob: Uint8Array,
    _blobInfo: LercHeaderInfo
  ) => { data: Uint8Array; maskData: Uint8Array };
} = {
  getBlobInfo: null,
  decode: null
};

function normalizeByteLength(n: number): number {
  // extra buffer on top of 8 byte boundary: https://stackoverflow.com/questions/56019003/why-malloc-in-webassembly-requires-4x-the-memory
  return ((n >> 3) << 3) + 16;
}

function initLercLib(lercFactory: LercFactory): void {
  const {
    _malloc,
    _free,
    _lerc_getBlobInfo,
    _lerc_getDataRanges,
    _lerc_decode,
    asm
  } = lercFactory;
  // do not use HeapU8 as memory dynamically grows from the initial 16MB
  // test case: landsat_6band_8bit.24
  let heapU8: Uint8Array;
  const memory = Object.values(asm).find(
    (val) => val && "buffer" in val && val.buffer === lercFactory.HEAPU8.buffer
  );
  const mallocMultiple = (byteLengths: number[]) => {
    // malloc once to avoid pointer for detached memory when it grows to allocate next chunk
    const lens = byteLengths.map((len) => normalizeByteLength(len));
    const byteLength = lens.reduce((a, b) => a + b);
    const ret = _malloc(byteLength);
    heapU8 = new Uint8Array(memory.buffer);
    let prev = lens[0];
    lens[0] = ret;
    // pointers for each allocated block
    for (let i = 1; i < lens.length; i++) {
      const next = lens[i];
      lens[i] = lens[i - 1] + prev;
      prev = next;
    }
    return lens;
  };
  lercLib.getBlobInfo = (blob: Uint8Array) => {
    // copy data to wasm. info: 10 * Uint32, range: 3 * F64
    const infoArr = new Uint8Array(10 * 4);
    const rangeArr = new Uint8Array(3 * 8);
    const [ptr, ptr_info, ptr_range] = mallocMultiple([
      blob.length,
      infoArr.length,
      rangeArr.length
    ]);
    heapU8.set(blob, ptr);
    heapU8.set(infoArr, ptr_info);
    heapU8.set(rangeArr, ptr_range);

    // decode
    let hr = _lerc_getBlobInfo(ptr, blob.length, ptr_info, ptr_range, 10, 3);
    if (hr) {
      _free(ptr);
      throw `lerc-getBlobInfo: error code is ${hr}`;
    }
    heapU8 = new Uint8Array(memory.buffer);
    infoArr.set(heapU8.slice(ptr_info, ptr_info + 10 * 4));
    rangeArr.set(heapU8.slice(ptr_range, ptr_range + 3 * 8));
    const lercInfoArr = new Uint32Array(infoArr.buffer);
    const statsArr = new Float64Array(rangeArr.buffer);
    const [
      version,
      dataType,
      dimCount,
      width,
      height,
      bandCount,
      validPixelCount,
      blobSize,
      maskCount
    ] = lercInfoArr;
    const headerInfo: LercHeaderInfo = {
      version,
      dimCount,
      width,
      height,
      validPixelCount,
      bandCount,
      blobSize,
      maskCount,
      dataType,
      minValue: statsArr[0],
      maxValue: statsArr[1],
      maxZerror: statsArr[2],
      statistics: []
    };
    if (dimCount === 1 && bandCount === 1) {
      _free(ptr);
      headerInfo.statistics.push({
        minValue: statsArr[0],
        maxValue: statsArr[1]
      });
      return headerInfo;
    }

    // get data ranges for nband / ndim blob
    // to reuse blob ptr we need to handle dynamic memory allocation
    const numStatsBytes = dimCount * bandCount * 8;
    const bandStatsMinArr = new Uint8Array(numStatsBytes);
    const bandStatsMaxArr = new Uint8Array(numStatsBytes);
    let ptr_blob = ptr,
      ptr_min = 0,
      ptr_max,
      blob_freed = false;
    if (heapU8.byteLength < ptr + numStatsBytes * 2) {
      _free(ptr);
      blob_freed = true;
      [ptr_blob, ptr_min, ptr_max] = mallocMultiple([
        blob.length,
        numStatsBytes,
        numStatsBytes
      ]);
      heapU8.set(blob, ptr_blob);
    } else {
      [ptr_min, ptr_max] = mallocMultiple([numStatsBytes, numStatsBytes]);
    }
    heapU8.set(bandStatsMinArr, ptr_min);
    heapU8.set(bandStatsMaxArr, ptr_max);
    hr = _lerc_getDataRanges(
      ptr_blob,
      blob.length,
      dimCount,
      bandCount,
      ptr_min,
      ptr_max
    );
    if (hr) {
      _free(ptr_blob);
      if (!blob_freed) {
        // we have two pointers in two wasm function calls
        _free(ptr_min);
      }
      throw `lerc-getDataRanges: error code is ${hr}`;
    }
    heapU8 = new Uint8Array(memory.buffer);
    bandStatsMinArr.set(heapU8.slice(ptr_min, ptr_min + numStatsBytes));
    bandStatsMaxArr.set(heapU8.slice(ptr_max, ptr_max + numStatsBytes));
    const allMinValues = new Float64Array(bandStatsMinArr.buffer);
    const allMaxValues = new Float64Array(bandStatsMaxArr.buffer);
    const statistics = headerInfo.statistics;
    for (let i = 0; i < bandCount; i++) {
      if (dimCount > 1) {
        const minValues = allMinValues.slice(i * dimCount, (i + 1) * dimCount);
        const maxValues = allMaxValues.slice(i * dimCount, (i + 1) * dimCount);
        const minValue = Math.min.apply(null, minValues);
        const maxValue = Math.max.apply(null, maxValues);
        statistics.push({
          minValue,
          maxValue,
          dimStats: { minValues, maxValues }
        });
      } else {
        statistics.push({
          minValue: allMinValues[i],
          maxValue: allMaxValues[i]
        });
      }
    }
    _free(ptr_blob);
    if (!blob_freed) {
      // we have two pointers in two wasm function calls
      _free(ptr_min);
    }
    return headerInfo;
  };
  lercLib.decode = (blob: Uint8Array, blobInfo: LercHeaderInfo) => {
    const { maskCount, dimCount, bandCount, width, height, dataType } =
      blobInfo;

    // if the heap is increased dynamically between raw data, mask, and data, the malloc pointer is invalid as it will raise error when accessing mask:
    // Cannot perform %TypedArray%.prototype.slice on a detached ArrayBuffer
    const pixelTypeInfo = pixelTypeInfoMap[dataType];
    const numPixels = width * height;
    const maskData = new Uint8Array(numPixels * bandCount);

    const numDataBytes = numPixels * dimCount * bandCount * pixelTypeInfo.size;
    const data = new Uint8Array(numDataBytes);

    const [ptr, ptr_mask, ptr_data] = mallocMultiple([
      blob.length,
      maskData.length,
      data.length
    ]);
    heapU8.set(blob, ptr);
    heapU8.set(maskData, ptr_mask);
    heapU8.set(data, ptr_data);

    const hr = _lerc_decode(
      ptr,
      blob.length,
      maskCount,
      ptr_mask,
      dimCount,
      width,
      height,
      bandCount,
      dataType,
      ptr_data
    );
    if (hr) {
      _free(ptr);
      throw `lerc-decode: error code is ${hr}`;
    }
    heapU8 = new Uint8Array(memory.buffer);
    data.set(heapU8.slice(ptr_data, ptr_data + numDataBytes));
    maskData.set(heapU8.slice(ptr_mask, ptr_mask + numPixels));
    _free(ptr);
    return {
      data,
      maskData
    };
  };
}

function swapDimensionOrder(
  pixels: PixelTypedArray,
  numPixels: number,
  numDims: number,
  OutPixelTypeArray: PixelTypedArrayCtor,
  inputIsBIP: boolean
): PixelTypedArray {
  if (numDims < 2) {
    return pixels;
  }
  const swap = new OutPixelTypeArray(numPixels * numDims);
  if (inputIsBIP) {
    for (let i = 0, j = 0; i < numPixels; i++) {
      for (let iDim = 0, temp = i; iDim < numDims; iDim++, temp += numPixels) {
        swap[temp] = pixels[j++];
      }
    }
  } else {
    for (let i = 0, j = 0; i < numPixels; i++) {
      for (let iDim = 0, temp = i; iDim < numDims; iDim++, temp += numPixels) {
        swap[j++] = pixels[temp];
      }
    }
  }
  return swap;
}

interface DecodeOptions {
  inputOffset?: number;
  returnPixelInterleavedDims?: boolean;
  noDataValue?: number;
  pixelType?: LercPixelType;
}

/**
 * Decoding a LERC1/LERC2 byte stream and return an object containing the pixel data.
 *
 * @alias module:Lerc
 * @param {ArrayBuffer} input The LERC input byte stream
 * @param {object} [options] The decoding options below are optional.
 * @param {number} [options.inputOffset] The number of bytes to skip in the input byte stream. A valid Lerc file is expected at that position.
 * @param {string} [options.pixelType] (LERC1 only) Default value is F32. Valid pixel types for input are U8/S8/S16/U16/S32/U32/F32.
 * @param {number} [options.noDataValue] (LERC1 only). It is recommended to use the returned mask instead of setting this value.
 * @param {boolean} [options.returnPixelInterleavedDims] (nDim LERC2 only) If true, returned dimensions are pixel-interleaved, a.k.a [p1_dim0, p1_dim1, p1_dimn, p2_dim0...], default is [p1_dim0, p2_dim0, ..., p1_dim1, p2_dim1...]
 * @returns {{width, height, pixels, pixelType, mask, statistics}}
 * @property {number} width Width of decoded image.
 * @property {number} height Height of decoded image.
 * @property {array} pixels [band1, band2, …] Each band is a typed array of width*height.
 * @property {string} pixelType The type of pixels represented in the output: U8/S8/S16/U16/S32/U32/F32.
 * @property {mask} mask Typed array with a size of width*height, or null if all pixels are valid.
 * @property {array} statistics [statistics_band1, statistics_band2, …] Each element is a statistics object representing min and max values
 **/
export function decode(
  input: ArrayBuffer,
  options: DecodeOptions = {}
): LercData {
  // get blob info
  const inputOffset = options.inputOffset ?? 0;
  const blob =
    input instanceof Uint8Array
      ? input.subarray(inputOffset)
      : new Uint8Array(input, inputOffset);

  const blobInfo = lercLib.getBlobInfo(blob);

  // decode
  const { data, maskData } = lercLib.decode(blob, blobInfo);

  const {
    width,
    height,
    bandCount,
    dimCount,
    dataType,
    maskCount,
    statistics
  } = blobInfo;

  // get pixels, per-band masks, and statistics
  const pixelTypeInfo = pixelTypeInfoMap[dataType];
  const data1 = new pixelTypeInfo.ctor(data.buffer);
  const pixels = [];
  const masks = [];
  const numPixels = width * height;
  const numElementsPerBand = numPixels * dimCount;
  for (let i = 0; i < bandCount; i++) {
    const band = data1.subarray(
      i * numElementsPerBand,
      (i + 1) * numElementsPerBand
    );
    if (options.returnPixelInterleavedDims) {
      pixels.push(band);
    } else {
      const bsq = swapDimensionOrder(
        band,
        numPixels,
        dimCount,
        pixelTypeInfo.ctor,
        true
      );
      pixels.push(bsq);
    }
    masks.push(
      maskData.subarray(i * numElementsPerBand, (i + 1) * numElementsPerBand)
    );
  }

  // get unified mask
  const mask =
    maskCount === 0
      ? null
      : maskCount === 1
      ? masks[0]
      : new Uint8Array(numPixels);

  if (maskCount > 1) {
    mask.set(masks[0]);
    for (let i = 1; i < masks.length; i++) {
      const bandMask = masks[i];
      for (let j = 0; j < numPixels; j++) {
        mask[j] = mask[j] & bandMask[j];
      }
    }
  }

  // apply no data value
  const { noDataValue } = options;
  const applyNoDataValue =
    noDataValue != null &&
    pixelTypeInfo.range[0] <= noDataValue &&
    pixelTypeInfo.range[1] >= noDataValue;
  if (maskCount > 0 && applyNoDataValue) {
    for (let i = 0; i < bandCount; i++) {
      const band = pixels[i];
      const bandMask = masks[i] || mask;
      for (let j = 0; j < numPixels; j++) {
        if (bandMask[j] === 0) {
          band[j] = noDataValue;
        }
      }
    }
  }

  // only keep band masks when there's per-band unique mask
  const bandMasks = maskCount === bandCount && bandCount > 1 ? masks : null;

  // lerc2.0 was never released
  const pixelType =
    options.pixelType && blobInfo.version === 0
      ? options.pixelType
      : pixelTypeInfo.pixelType;

  return {
    width,
    height,
    bandCount,
    pixelType,
    dimCount,
    statistics,
    pixels,
    mask,
    bandMasks
  };
}
/**
 * Get the header information of a LERC1/LERC2 byte stream.
 *
 * @alias module:Lerc
 * @param {ArrayBuffer} input The LERC input byte stream
 * @param {object} [options] The decoding options below are optional.
 * @param {number} [options.inputOffset] The number of bytes to skip in the input byte stream. A valid Lerc file is expected at that position.
 * @returns {{version, width, height, bandCount, dimCount, validPixelCount, blobSize, dataType, mask, minValue, maxValue, maxZerror, statistics}}
 * @property {number} version Compression algorithm version.
 * @property {number} width Width of decoded image.
 * @property {number} height Height of decoded image.
 * @property {number} bandCount Number of bands.
 * @property {number} dimCount Number of dimensions.
 * @property {number} validPixelCount Number of valid pixels.
 * @property {number} blobSize Lerc blob size in bytes.
 * @property {number} dataType Data type represented in number.
 * @property {number} minValue Minimum pixel value.
 * @property {number} maxValue Maximum pixel value.
 * @property {number} maxZerror Maximum Z error.
 * @property {array} statistics [statistics_band1, statistics_band2, …] Each element is a statistics object representing min and max values
 **/
export function getBlobInfo(
  input: ArrayBuffer,
  options: { inputOffset?: number } = {}
): LercHeaderInfo {
  const blob = new Uint8Array(input, options.inputOffset ?? 0);
  return lercLib.getBlobInfo(blob);
}

export function getBandCount(
  input: ArrayBuffer | Uint8Array,
  options: { inputOffset?: number } = {}
): number {
  // this was available in the old JS version but not documented. Keep as is for backward compatiblity
  const info = getBlobInfo(input, options);
  return info.bandCount;
}
