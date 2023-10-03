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

type LercPixelType = "S8" | "U8" | "S16" | "U16" | "S32" | "U32" | "F32" | "F64";

interface BandStats {
  minValue: number;
  maxValue: number;
  /**
   * deprecated, will be removed in next release. use depthStats instead
   */
  dimStats?: {
    minValues: Float64Array;
    maxValues: Float64Array;
  };
  depthStats?: {
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
  depthCount: number;
  dataType: number;
  minValue: number;
  maxValue: number;
  maxZerror: number;
  statistics: BandStats[];
  bandCountWithNoData: number;
}

interface DecodeOptions {
  inputOffset?: number;
  returnInterleaved?: boolean;
  /**
   * deprecated, will be removed in next release. use returnInterleaved instead
   */
  returnPixelInterleavedDims?: boolean;
  noDataValue?: number;
}

interface LercData {
  width: number;
  height: number;
  pixelType: LercPixelType;
  statistics: BandStats[];
  pixels: PixelTypedArray[];
  mask: Uint8Array;
  dimCount: number;
  depthCount: number;
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
  const locateFile = options.locateFile || ((wasmFileName: string, scriptDir: string) => `${scriptDir}${wasmFileName}`);
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
  getBlobInfo: (blob: Uint8Array) => LercHeaderInfo;
  decode: (
    blob: Uint8Array,
    blobInfo: LercHeaderInfo
  ) => {
    data: Uint8Array;
    maskData: Uint8Array;
    noDataValues: (number | null)[] | null;
  };
} = {
  getBlobInfo: null,
  decode: null
};

function normalizeByteLength(n: number): number {
  // extra buffer on top of 8 byte boundary: https://stackoverflow.com/questions/56019003/why-malloc-in-webassembly-requires-4x-the-memory
  return ((n >> 3) << 3) + 16;
}

function copyBytesFromWasm(wasmHeapU8: Uint8Array, ptr_data: number, data: Uint8Array): void {
  data.set(wasmHeapU8.slice(ptr_data, ptr_data + data.length));
}

function initLercLib(lercFactory: LercFactory): void {
  const { _malloc, _free, _lerc_getBlobInfo, _lerc_getDataRanges, _lerc_decode_4D, asm } = lercFactory;
  // do not use HeapU8 as memory dynamically grows from the initial 16MB
  // test case: landsat_6band_8bit.24
  let heapU8: Uint8Array;
  const memory = Object.values(asm).find((val) => val && "buffer" in val && val.buffer === lercFactory.HEAPU8.buffer);
  // avoid pointer for detached memory, malloc once:
  const mallocMultiple = (byteLengths: number[]) => {
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
    // copy data to wasm. info: Uint32, range: F64
    const infoArrSize = 12;
    const rangeArrSize = 3;
    const infoArr = new Uint8Array(infoArrSize * 4);
    const rangeArr = new Uint8Array(rangeArrSize * 8);
    const [ptr, ptr_info, ptr_range] = mallocMultiple([blob.length, infoArr.length, rangeArr.length]);
    heapU8.set(blob, ptr);
    heapU8.set(infoArr, ptr_info);
    heapU8.set(rangeArr, ptr_range);

    // decode
    let hr = _lerc_getBlobInfo(ptr, blob.length, ptr_info, ptr_range, infoArrSize, rangeArrSize);
    if (hr) {
      _free(ptr);
      throw `lerc-getBlobInfo: error code is ${hr}`;
    }
    heapU8 = new Uint8Array(memory.buffer);
    copyBytesFromWasm(heapU8, ptr_info, infoArr);
    copyBytesFromWasm(heapU8, ptr_range, rangeArr);
    const lercInfoArr = new Uint32Array(infoArr.buffer);
    const statsArr = new Float64Array(rangeArr.buffer);
    // skip ndepth
    const [
      version,
      dataType,
      dimCount,
      width,
      height,
      bandCount,
      validPixelCount,
      blobSize,
      maskCount,
      depthCount,
      bandCountWithNoData
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
      depthCount,
      dataType,
      minValue: statsArr[0],
      maxValue: statsArr[1],
      maxZerror: statsArr[2],
      statistics: [],
      bandCountWithNoData
    };
    if (bandCountWithNoData) {
      _free(ptr);
      return headerInfo;
    }
    if (depthCount === 1 && bandCount === 1) {
      _free(ptr);
      headerInfo.statistics.push({
        minValue: statsArr[0],
        maxValue: statsArr[1]
      });
      return headerInfo;
    }

    // get data ranges for nband / ndim blob
    // to reuse blob ptr we need to handle dynamic memory allocation
    const numStatsBytes = depthCount * bandCount * 8;
    const bandStatsMinArr = new Uint8Array(numStatsBytes);
    const bandStatsMaxArr = new Uint8Array(numStatsBytes);
    let ptr_blob = ptr,
      ptr_min = 0,
      ptr_max = 0,
      blob_freed = false;
    if (heapU8.byteLength < ptr + numStatsBytes * 2) {
      _free(ptr);
      blob_freed = true;
      [ptr_blob, ptr_min, ptr_max] = mallocMultiple([blob.length, numStatsBytes, numStatsBytes]);
      heapU8.set(blob, ptr_blob);
    } else {
      [ptr_min, ptr_max] = mallocMultiple([numStatsBytes, numStatsBytes]);
    }
    heapU8.set(bandStatsMinArr, ptr_min);
    heapU8.set(bandStatsMaxArr, ptr_max);
    hr = _lerc_getDataRanges(ptr_blob, blob.length, depthCount, bandCount, ptr_min, ptr_max);
    if (hr) {
      _free(ptr_blob);
      if (!blob_freed) {
        // we have two pointers in two wasm function calls
        _free(ptr_min);
      }
      throw `lerc-getDataRanges: error code is ${hr}`;
    }
    heapU8 = new Uint8Array(memory.buffer);
    copyBytesFromWasm(heapU8, ptr_min, bandStatsMinArr);
    copyBytesFromWasm(heapU8, ptr_max, bandStatsMaxArr);
    const allMinValues = new Float64Array(bandStatsMinArr.buffer);
    const allMaxValues = new Float64Array(bandStatsMaxArr.buffer);
    const statistics = headerInfo.statistics;
    for (let i = 0; i < bandCount; i++) {
      if (depthCount > 1) {
        const minValues = allMinValues.slice(i * depthCount, (i + 1) * depthCount);
        const maxValues = allMaxValues.slice(i * depthCount, (i + 1) * depthCount);
        const minValue = Math.min.apply(null, minValues);
        const maxValue = Math.max.apply(null, maxValues);
        statistics.push({
          minValue,
          maxValue,
          dimStats: { minValues, maxValues },
          depthStats: { minValues, maxValues }
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
    const { maskCount, depthCount, bandCount, width, height, dataType, bandCountWithNoData } = blobInfo;

    // if the heap is increased dynamically between raw data, mask, and data, the malloc pointer is invalid as it will raise error when accessing mask:
    // Cannot perform %TypedArray%.prototype.slice on a detached ArrayBuffer
    const pixelTypeInfo = pixelTypeInfoMap[dataType];
    const numPixels = width * height;
    const maskData = new Uint8Array(numPixels * bandCount);
    const numDataBytes = numPixels * depthCount * bandCount * pixelTypeInfo.size;
    const data = new Uint8Array(numDataBytes);
    const useNoDataArr = new Uint8Array(bandCount);
    const noDataArr = new Uint8Array(bandCount * 8);

    const [ptr, ptr_mask, ptr_data, ptr_useNoData, ptr_noData] = mallocMultiple([
      blob.length,
      maskData.length,
      data.length,
      useNoDataArr.length,
      noDataArr.length
    ]);
    heapU8.set(blob, ptr);
    heapU8.set(maskData, ptr_mask);
    heapU8.set(data, ptr_data);
    heapU8.set(useNoDataArr, ptr_useNoData);
    heapU8.set(noDataArr, ptr_noData);

    const hr = _lerc_decode_4D(
      ptr,
      blob.length,
      maskCount,
      ptr_mask,
      depthCount,
      width,
      height,
      bandCount,
      dataType,
      ptr_data,
      ptr_useNoData,
      ptr_noData
    );
    if (hr) {
      _free(ptr);
      throw `lerc-decode: error code is ${hr}`;
    }
    heapU8 = new Uint8Array(memory.buffer);
    copyBytesFromWasm(heapU8, ptr_data, data);
    copyBytesFromWasm(heapU8, ptr_mask, maskData);
    let noDataValues: (number | null)[] = null;
    if (bandCountWithNoData) {
      copyBytesFromWasm(heapU8, ptr_useNoData, useNoDataArr);
      copyBytesFromWasm(heapU8, ptr_noData, noDataArr);
      noDataValues = [];
      const noDataArr64 = new Float64Array(noDataArr.buffer);
      for (let i = 0; i < useNoDataArr.length; i++) {
        noDataValues.push(useNoDataArr[i] ? noDataArr64[i] : null);
      }
    }

    _free(ptr);
    return {
      data,
      maskData,
      noDataValues
    };
  };
}

function swapDepthValuesOrder(
  pixels: PixelTypedArray,
  numPixels: number,
  depthCount: number,
  OutPixelTypeArray: PixelTypedArrayCtor,
  inputIsBIP: boolean
): PixelTypedArray {
  if (depthCount < 2) {
    return pixels;
  }
  const swap = new OutPixelTypeArray(numPixels * depthCount);
  if (inputIsBIP) {
    for (let i = 0, j = 0; i < numPixels; i++) {
      for (let iDim = 0, temp = i; iDim < depthCount; iDim++, temp += numPixels) {
        swap[temp] = pixels[j++];
      }
    }
  } else {
    for (let i = 0, j = 0; i < numPixels; i++) {
      for (let iDim = 0, temp = i; iDim < depthCount; iDim++, temp += numPixels) {
        swap[j++] = pixels[temp];
      }
    }
  }
  return swap;
}

/**
 * Decoding a LERC1/LERC2 byte stream and return an object containing the pixel data.
 *
 * @alias module:Lerc
 * @param {ArrayBuffer | Uint8Array} input The LERC input byte stream
 * @param {object} [options] The decoding options below are optional.
 * @param {number} [options.inputOffset] The number of bytes to skip in the input byte stream. A valid Lerc file is expected at that position.
 * @param {number} [options.noDataValue] It is recommended to use the returned mask instead of setting this value.
 * @param {boolean} [options.returnInterleaved] (ndepth LERC2 only) If true, returned depth values are pixel-interleaved, a.k.a [p1_dep1, p1_dep2, ..., p1_depN, p2_dep1...], default is [p1_dep1, p2_dep1, ..., p1_dep2, p2_dep2...]
 * @returns {{width, height, pixels, pixelType, mask, statistics}}
 * @property {number} width Width of decoded image.
 * @property {number} height Height of decoded image.
 * @property {number} depthCount Depth count.
 * @property {array} pixels [band1, band2, …] Each band is a typed array of width*height*depthCount.
 * @property {string} pixelType The type of pixels represented in the output: U8/S8/S16/U16/S32/U32/F32.
 * @property {mask} mask Typed array with a size of width*height, or null if all pixels are valid.
 * @property {array} statistics [statistics_band1, statistics_band2, …] Each element is a statistics object representing min and max values
 * @property {array} [bandMasks] [band1_mask, band2_mask, …] Each band is a Uint8Array of width * height * depthCount.
 **/
export function decode(input: ArrayBuffer | Uint8Array, options: DecodeOptions = {}): LercData {
  // get blob info
  const inputOffset = options.inputOffset ?? 0;
  const blob = input instanceof Uint8Array ? input.subarray(inputOffset) : new Uint8Array(input, inputOffset);
  const blobInfo = lercLib.getBlobInfo(blob);

  // decode
  const { data, maskData } = lercLib.decode(blob, blobInfo);
  const { width, height, bandCount, dimCount, depthCount, dataType, maskCount, statistics } = blobInfo;

  // get pixels, per-band masks, and statistics
  const pixelTypeInfo = pixelTypeInfoMap[dataType];
  const data1 = new pixelTypeInfo.ctor(data.buffer);
  const pixels = [];
  const masks = [];
  const numPixels = width * height;
  const numElementsPerBand = numPixels * depthCount;
  // options.returnPixelInterleavedDims will be removed in next release
  const swap = options.returnInterleaved ?? options.returnPixelInterleavedDims;
  for (let i = 0; i < bandCount; i++) {
    const band = data1.subarray(i * numElementsPerBand, (i + 1) * numElementsPerBand);
    if (swap) {
      pixels.push(band);
    } else {
      const bsq = swapDepthValuesOrder(band, numPixels, depthCount, pixelTypeInfo.ctor, true);
      pixels.push(bsq);
    }
    masks.push(maskData.subarray(i * numElementsPerBand, (i + 1) * numElementsPerBand));
  }

  // get unified mask
  const mask = maskCount === 0 ? null : maskCount === 1 ? masks[0] : new Uint8Array(numPixels);
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
    noDataValue != null && pixelTypeInfo.range[0] <= noDataValue && pixelTypeInfo.range[1] >= noDataValue;
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

  const { pixelType } = pixelTypeInfo;

  return {
    width,
    height,
    pixelType,
    statistics,
    pixels,
    mask,
    dimCount,
    depthCount,
    bandMasks
  };
}

/**
 * Get the header information of a LERC1/LERC2 byte stream.
 *
 * @alias module:Lerc
 * @param {ArrayBuffer | Uint8Array} input The LERC input byte stream
 * @param {object} [options] The decoding options below are optional.
 * @param {number} [options.inputOffset] The number of bytes to skip in the input byte stream. A valid Lerc file is expected at that position.
 * @returns {{version, width, height, bandCount, dimCount, validPixelCount, blobSize, dataType, mask, minValue, maxValue, maxZerror, statistics}}
 * @property {number} version Compression algorithm version.
 * @property {number} width Width of decoded image.
 * @property {number} height Height of decoded image.
 * @property {number} bandCount Number of bands.
 * @property {number} depthCount Depth count.
 * @property {number} validPixelCount Number of valid pixels.
 * @property {number} blobSize Lerc blob size in bytes.
 * @property {number} dataType Data type represented in number.
 * @property {number} minValue Minimum pixel value.
 * @property {number} maxValue Maximum pixel value.
 * @property {number} maxZerror Maximum Z error.
 * @property {array} statistics [statistics_band1, statistics_band2, …] Each element is a statistics object representing min and max values
 **/
export function getBlobInfo(input: ArrayBuffer | Uint8Array, options: { inputOffset?: number } = {}): LercHeaderInfo {
  const inputOffset = options.inputOffset ?? 0;
  const blob = input instanceof Uint8Array ? input.subarray(inputOffset) : new Uint8Array(input, inputOffset);
  return lercLib.getBlobInfo(blob);
}

export function getBandCount(input: ArrayBuffer | Uint8Array, options: { inputOffset?: number } = {}): number {
  // this was available in the old JS version but not documented. Keep as is for backward compatibility.
  const info = getBlobInfo(input, options);
  return info.bandCount;
}
