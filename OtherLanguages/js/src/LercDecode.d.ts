export type PixelTypedArray =
  | Int8Array
  | Uint8Array
  | Uint8ClampedArray
  | Int16Array
  | Uint16Array
  | Int32Array
  | Uint32Array
  | Float32Array
  | Float64Array;

export type LercPixelType = "S8" | "U8" | "S16" | "U16" | "S32" | "U32" | "F32" | "F64";

export interface BandStats {
  minValue: number;
  maxValue: number;
  depthStats?: {
    minValues: Float64Array;
    maxValues: Float64Array;
  };
}

export interface LercHeaderInfo {
  version: number;
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

export interface DecodeOptions {
  inputOffset?: number;
  returnInterleaved?: boolean;
  noDataValue?: number;
}

export interface LercData {
  width: number;
  height: number;
  pixelType: LercPixelType;
  statistics: BandStats[];
  pixels: PixelTypedArray[];
  mask: Uint8Array;
  depthCount: number;
  bandMasks?: Uint8Array[];
  noDataValues: (number | null)[] | null;
}

/**
 * Load the LERC wasm module.
 *
 * @param options Use the options to specify a function to locate the wasm file, if not located in the same directory as the script.
 */
export function load(options?: { locateFile?: (wasmFileName?: string, scriptDir?: string) => string }): Promise<void>;

export function isLoaded(): boolean;

/**
 * Decode a LERC byte stream and return an object containing the pixel data.
 *
 * @param {object} [options] The decoding options below are optional.
 * @param {number} [options.inputOffset] The number of bytes to skip in the input byte stream. A valid Lerc file is expected at that position.
 * @param {number} [options.noDataValue] It is recommended to use the returned mask instead of setting this value.
 * @param {boolean} [options.returnInterleaved] (ndepth LERC2 only) If true, returned depth values are pixel-interleaved, a.k.a [p1_dep1, p1_dep2, ..., p1_depN, p2_dep1...], default is [p1_dep1, p2_dep1, ..., p1_dep2, p2_dep2...]
 *
 */
export function decode(input: ArrayBuffer | Uint8Array, options?: DecodeOptions): LercData;

export function getBlobInfo(input: ArrayBuffer | Uint8Array, options?: { inputOffset?: number }): LercHeaderInfo;

/**
 * @deprecated Use getBlobInfo() instead.
 */
export function getBandCount(input: ArrayBuffer | Uint8Array, options?: { inputOffset?: number }): number;
