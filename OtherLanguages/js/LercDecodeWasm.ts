type PixelArrayConstructor =
  | Int8ArrayConstructor
  | Uint8ArrayConstructor
  | Int16ArrayConstructor
  | Uint16ArrayConstructor
  | Int32ArrayConstructor
  | Uint32ArrayConstructor
  | Float32ArrayConstructor
  | Float64ArrayConstructor;

type PixelType = "S8" | "U8" | "S16" | "U16" | "S32" | "U32" | "F32" | "F64";

type PixelArray =
  | Int8Array
  | Uint8Array
  | Int16Array
  | Uint16Array
  | Int32Array
  | Uint32Array
  | Float32Array
  | Float64Array;

const pixelTypeInfoMap = new Map<
  number,
  { pixelType: PixelType; size: number; ctor: PixelArrayConstructor }
>();
pixelTypeInfoMap.set(0, { pixelType: "S8", size: 1, ctor: Int8Array });
pixelTypeInfoMap.set(1, { pixelType: "U8", size: 1, ctor: Uint8Array });
pixelTypeInfoMap.set(2, { pixelType: "S16", size: 2, ctor: Int16Array });
pixelTypeInfoMap.set(3, { pixelType: "U16", size: 2, ctor: Uint16Array });
pixelTypeInfoMap.set(4, { pixelType: "S32", size: 3, ctor: Int32Array });
pixelTypeInfoMap.set(5, { pixelType: "U32", size: 3, ctor: Uint32Array });
pixelTypeInfoMap.set(6, { pixelType: "F32", size: 4, ctor: Float32Array });
pixelTypeInfoMap.set(7, { pixelType: "F64", size: 8, ctor: Float64Array });

function swapDimensionOrder(
  pixels: PixelArray,
  numPixels: number,
  numDims: number,
  OutPixelTypeArray: PixelArrayConstructor,
  inputIsBIP: boolean
): PixelArray {
  if (numDims <= 1) {
    return pixels;
  }
  const swap = new OutPixelTypeArray(numPixels * numDims);
  if (inputIsBIP) {
    for (let i = 0, j = 0; i < numPixels; i++) {
      let temp = i;
      for (let iDim = 0; iDim < numDims; iDim++, temp += numPixels) {
        swap[temp] = pixels[j++];
      }
    }
  } else {
    for (let i = 0, j = 0; i < numPixels; i++) {
      let temp = i;
      for (let iDim = 0; iDim < numDims; iDim++, temp += numPixels) {
        swap[j++] = pixels[temp];
      }
    }
  }
  return swap;
}
