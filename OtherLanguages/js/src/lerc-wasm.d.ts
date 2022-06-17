export interface LercFactory {
  HEAPU8: Uint8Array;
  _malloc: (ptr: number) => number;
  _free: (ptr: number) => number;
  _lerc_getBlobInfo: (
    ptr: number,
    blobLength: number,
    ptr_info: number,
    ptr_range: number,
    infoArrLength: number,
    rangeArrLength: number
  ) => number;
  _lerc_getDataRanges: (
    ptr: number,
    blobLength: number,
    depthCount: number,
    bandCount: number,
    ptr_minArr: number,
    ptr_maxArr: number
  ) => number;
  _lerc_decode_4D: (
    ptr: number,
    blobSize: number,
    maskCount: number,
    ptr_mask: number,
    depthCount: number,
    width: number,
    height: number,
    bandCount: number,
    dataType: number,
    ptr_data: number,
    ptr_useNoData: number,
    ptr_noData: number
  ) => number;
  asm: Record<string, any>;
  ready: Promise<void>;
}

export default function (options: object): Promise<LercFactory>;
