import lercWasm from "./lerc.js";

const pixelTypeInfoMap = new Map();
pixelTypeInfoMap.set(0, {
  pixelType: "S8",
  size: 1,
  ctor: Int8Array,
  range: [-128, 128]
});
pixelTypeInfoMap.set(1, {
  pixelType: "U8",
  size: 1,
  ctor: Uint8Array,
  range: [0, 255]
});
pixelTypeInfoMap.set(2, {
  pixelType: "S16",
  size: 2,
  ctor: Int16Array,
  range: [-32768, 32767]
});
pixelTypeInfoMap.set(3, {
  pixelType: "U16",
  size: 2,
  ctor: Uint16Array,
  range: [0, 65536]
});
pixelTypeInfoMap.set(4, {
  pixelType: "S32",
  size: 4,
  ctor: Int32Array,
  range: [-2147483648, 2147483647]
});
pixelTypeInfoMap.set(5, {
  pixelType: "U32",
  size: 4,
  ctor: Uint32Array,
  range: [0, 4294967296]
});
pixelTypeInfoMap.set(6, {
  pixelType: "F32",
  size: 4,
  ctor: Float32Array,
  range: [-3.4027999387901484e38, 3.4027999387901484e38]
});
pixelTypeInfoMap.set(7, {
  pixelType: "F64",
  size: 8,
  ctor: Float64Array,
  range: [-1.7976931348623157e308, 1.7976931348623157e308]
});

let loadPromise = null;
let loaded = false;
export function load() {
  if (loadPromise) {
    return loadPromise;
  }
  loadPromise = lercWasm().then((lercFactory) =>
    lercFactory.ready.then(() => {
      initLercDecode(lercFactory);
      loaded = true;
    })
  );
  return loadPromise;
}

export function isLoaded() {
  return loaded;
}

export const lercWrapper = {
  getBlobInfo: () => {},
  decode: () => {}
};

function normalizeByteLength(n) {
  // 8 byte boundary: https://stackoverflow.com/questions/56019003/why-malloc-in-webassembly-requires-4x-the-memory
  return ((n >> 3) << 3) + 16;
}

function initLercDecode(lercFactory) {
  const {
    stackSave,
    stackRestore,
    stackAlloc,
    _malloc,
    _free,
    _lerc_getBlobInfo,
    _lerc_decode,
    asm: { memory }
  } = lercFactory;
  // do not use HeapU8 as memory dynamically grows from the initial 16MB.
  // test case: landsat_6band_8bit.24
  let heapU8;
  function malloc(...args) {
    const lens = args.map((len) => normalizeByteLength(len));
    const byteLength = lens.reduce((a, b) => a + b);
    const ret = _malloc(byteLength);
    heapU8 = new Uint8Array(memory.buffer);
    let prev = lens[0];
    lens[0] = ret;
    for (let i = 1; i < lens.length; i++) {
      const next = lens[i];
      lens[i] = lens[i - 1] + prev;
      prev = next;
    }
    return lens;
  }
  lercWrapper.getBlobInfo = (blob) => {
    // TODO: does stackSave do anything?
    const stack = stackSave();

    // 10 * Uint32
    const infoArr = new Uint8Array(10 * 4);
    // 3 * F64
    const rangeArr = new Uint8Array(3 * 8);

    const [ptr, ptr_info, ptr_range] = malloc(
      blob.length,
      infoArr.length,
      rangeArr.length
    );
    heapU8.set(blob, ptr);
    heapU8.set(infoArr, ptr_info);
    heapU8.set(rangeArr, ptr_range);

    const hr = _lerc_getBlobInfo(ptr, blob.length, ptr_info, ptr_range, 10, 3);
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
    //   int version,        // Lerc version number (0 for old Lerc1, 1 to 4 for Lerc 2.1 to 2.4)
    //   nDim,             // number of values per pixel
    //   nCols,            // number of columns
    //   nRows,            // number of rows
    //   numValidPixel,    // number of valid pixels
    //   nBands,           // number of bands
    //   blobSize,         // total blob size in bytes
    //   nMasks;           // number of masks (0, 1, or nBands)
    // DataType dt;        // data type (float only for old Lerc1)
    // double zMin,        // min pixel value, over all data values
    //   zMax,             // max pixel value, over all data values
    //   maxZError;        // maxZError used for encoding
    stackRestore(stack);
    _free(ptr);
    return {
      info: {
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
        maxZerror
      },
      dataRange: rangeArr
    };
  };
  lercWrapper.decode = (blob, blobInfo) => {
    blobInfo = blobInfo || lercWrapper.getBlobInfo(blob).info;
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

    const pixelTypeInfo = pixelTypeInfoMap.get(dataType);
    const numPixels = width * height;
    const maskData = new Uint8Array(numPixels * bandCount);

    const numDataBytes = numPixels * dimCount * bandCount * pixelTypeInfo.size;
    const data = new Uint8Array(numDataBytes);

    // avoid pointer for detached memory, malloc once:

    const [ptr, ptr_mask, ptr_data] = malloc(
      blob.length,
      maskData.length,
      data.length
    );
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
      blobInfo,
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

  // decode
  const {
    blobInfo: {
      width,
      height,
      bandCount,
      dimCount,
      dataType,
      maskCount,
      minValue,
      maxValue
    },
    data,
    maskData
  } = lercWrapper.decode(blob);

  // get pixels, per-band masks, and statistics
  const pixelTypeInfo = pixelTypeInfoMap.get(dataType);
  const data1 = new pixelTypeInfo.ctor(data.buffer);
  const pixels = [];
  const masks = [];
  const statistics =
    bandCount > 1 ? readBandStats(blob.buffer, blob.byteOffset, bandCount) : [];
  const hasBandStatistics = statistics.length > 1;
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
    if (!hasBandStatistics) {
      statistics.push({ minValue, maxValue });
    }
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
