import lercWasm from "./lerc.js";

const pixelTypeInfoMap = [
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

let loadPromise = null;
let loaded = false;
export function load() {
  if (loadPromise) {
    return loadPromise;
  }
  loadPromise = lercWasm().then((lercFactory) =>
    lercFactory.ready.then(() => {
      initLercLib(lercFactory);
      loaded = true;
    })
  );
  return loadPromise;
}

export function isLoaded() {
  return loaded;
}

const lercWrapper = {
  getBlobInfo: () => {},
  decode: () => {}
};

function normalizeByteLength(n) {
  // extra buffer on top of 8 byte boundary: https://stackoverflow.com/questions/56019003/why-malloc-in-webassembly-requires-4x-the-memory
  return ((n >> 3) << 3) + 16;
}

function initLercLib(lercFactory) {
  const {
    stackSave,
    stackRestore,
    _malloc,
    _free,
    _lerc_getBlobInfo,
    _lerc_getDataRanges,
    _lerc_decode,
    asm: { memory }
  } = lercFactory;
  // do not use HeapU8 as memory dynamically grows from the initial 16MB.
  // test case: landsat_6band_8bit.24
  let heapU8;
  const malloc = (byteLengths) => {
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
  lercWrapper.getBlobInfo = (blob) => {
    // TODO: does stackSave do anything?
    const stack = stackSave();

    // 10 * Uint32
    const infoArr = new Uint8Array(10 * 4);
    // 3 * F64
    const rangeArr = new Uint8Array(3 * 8);

    const [ptr, ptr_info, ptr_range] = malloc([
      blob.length,
      infoArr.length,
      rangeArr.length
    ]);
    heapU8.set(blob, ptr);
    heapU8.set(infoArr, ptr_info);
    heapU8.set(rangeArr, ptr_range);

    let hr = _lerc_getBlobInfo(ptr, blob.length, ptr_info, ptr_range, 10, 3);
    if (hr) {
      throw `lerc-getBlobInfo: error code is ${hr}`;
    }
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
    const [minValue, maxValue, maxZerror] = statsArr;

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
      [ptr_blob, ptr_min, ptr_max] = malloc([
        blob.length,
        numStatsBytes,
        numStatsBytes
      ]);
      heapU8.set(blob, ptr_blob);
    } else {
      [ptr_min, ptr_max] = malloc([numStatsBytes, numStatsBytes]);
    }
    heapU8.set(bandStatsMinArr, ptr_min);
    heapU8.set(bandStatsMaxArr, ptr_max);
    hr = _lerc_getDataRanges(
      ptr,
      blob.length,
      dimCount,
      bandCount,
      ptr_min,
      ptr_max
    );
    if (hr) {
      throw `lerc-getDataRanges: error code is ${hr}`;
    }
    bandStatsMinArr.set(heapU8.slice(ptr_min, ptr_min + numStatsBytes));
    bandStatsMaxArr.set(heapU8.slice(ptr_max, ptr_max + numStatsBytes));
    const allMinValues = new Float64Array(bandStatsMinArr.buffer);
    const allMaxValues = new Float64Array(bandStatsMaxArr.buffer);
    const statistics = [];
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
    stackRestore(stack);
    _free(ptr_blob);
    // we have two pointers in two wasm function calls
    if (!blob_freed) {
      _free(ptr_min);
    }
    return {
      version,
      dimCount,
      width,
      height,
      validPixelCount,
      bandCount,
      blobSize,
      maskCount,
      dataType,
      minValue,
      maxValue,
      maxZerror,
      statistics
    };
  };
  lercWrapper.decode = (blob, blobInfo) => {
    const {
      blobSize,
      maskCount,
      dimCount,
      bandCount,
      width,
      height,
      dataType
    } = blobInfo;

    // if the heap is increased dynamically between raw data, mask, and data, the malloc pointer is invalid as it will raise error when accessing mask:
    // Cannot perform %TypedArray%.prototype.slice on a detached ArrayBuffer
    const stack = stackSave();

    const pixelTypeInfo = pixelTypeInfoMap[dataType];
    const numPixels = width * height;
    const maskData = new Uint8Array(numPixels * bandCount);

    const numDataBytes = numPixels * dimCount * bandCount * pixelTypeInfo.size;
    const data = new Uint8Array(numDataBytes);

    // avoid pointer for detached memory, malloc once:

    const [ptr, ptr_mask, ptr_data] = malloc([
      blob.length,
      maskData.length,
      data.length
    ]);
    heapU8.set(blob, ptr);
    heapU8.set(maskData, ptr_mask);
    heapU8.set(data, ptr_data);

    const hr = _lerc_decode(
      ptr,
      blobSize,
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
      throw `lerc-decode: error code is ${hr}`;
    }
    heapU8 = new Uint8Array(memory.buffer);
    data.set(heapU8.slice(ptr_data, ptr_data + numDataBytes));
    maskData.set(heapU8.slice(ptr_mask, ptr_mask + numPixels));
    stackRestore(stack);
    _free(ptr);
    return {
      data,
      maskData
    };
  };
}

function swapDimensionOrder(
  pixels,
  numPixels,
  numDims,
  OutPixelTypeArray,
  inputIsBIP
) {
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

function readBandStats(input, offset, bandCount) {
  const stats = [];
  while (offset < input.byteLength - 58) {
    let ptr = offset;
    // identifier
    const fileIdView = new Uint8Array(input, ptr, 6);
    const identifier = String.fromCharCode.apply(null, fileIdView);
    if (identifier.lastIndexOf("Lerc2", 0) !== 0) {
      break;
    }
    // version
    ptr += 6;
    let view = new DataView(input, ptr, 8);
    const fileVersion = view.getInt32(0, true);
    ptr += fileVersion >= 3 ? 8 : 4;
    view = new DataView(input, ptr, 12);
    const numDims = fileVersion >= 4 ? view.getUint32(8, true) : 1;
    ptr += fileVersion >= 4 ? 12 : 8;
    view = new DataView(input, ptr, 40);
    const blobSize = view.getInt32(8, true);
    stats.push({
      // maxZError = view.getFloat64(16, true)
      minValue: view.getFloat64(24, true),
      maxValue: view.getFloat64(32, true)
    });
    offset += blobSize;
  }
  // validation
  if (bandCount && bandCount !== stats.length) {
    return [];
  }
  return stats;
}

export function decode(input, options = {}) {
  // get blob info
  const inputOffset = options.inputOffset ?? 0;
  const blob =
    input instanceof Uint8Array
      ? input.subarray(inputOffset)
      : new Uint8Array(input, inputOffset);

  const blobInfo = lercWrapper.getBlobInfo(blob);

  // decode
  const { data, maskData } = lercWrapper.decode(blob, blobInfo);

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
  const statistics2 = readBandStats(blob.buffer, blob.byteOffset, bandCount);
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
    const { noDataValue } = options;
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

  return {
    width,
    height,
    bandCount,
    pixelType: pixelTypeInfo.pixelType,
    dimCount,
    statistics,
    statistics2,
    pixels,
    bandMasks
  };
}

export function getBandCount(input, options = {}) {
  const blob =
    input instanceof Uint8Array
      ? input.subarray(options.inputOffset ?? 0)
      : new Uint8Array(input, options.inputOffset ?? 0);
  const result = lercWrapper.getBlobInfo(blob);
  return result.bandCount;
}
