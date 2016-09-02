/* jshint forin: false, bitwise: false

Copyright 2015 Esri

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

A copy of the license and additional notices are located with the
source distribution at:

http://github.com/Esri/lerc/

Contributors:  Johannes Schmid,
               Chayanika Khatua,
               Wenxue Ju

*/


(function() {
  //the original LercCodec for Version 1.
  var LercCodec = (function() {

    // WARNING: This decoder version can only read old version 1 Lerc blobs. Use with caution.
    // A new, updated js Lerc decoder is in the works.

    // Note: currently, this module only has an implementation for decoding LERC data, not encoding. The name of
    // the class was chosen to be future proof.

    var CntZImage = {};

    CntZImage.defaultNoDataValue = -3.4027999387901484e+38; // smallest Float32 value

    /**
     * Decode a LERC byte stream and return an object containing the pixel data and some required and optional
     * information about it, such as the image's width and height.
     *
     * @param {ArrayBuffer} input The LERC input byte stream
     * @param {object} [options] Decoding options, containing any of the following properties:
     * @config {number} [inputOffset = 0]
     *        Skip the first inputOffset bytes of the input byte stream. A valid LERC file is expected at that position.
     * @config {Uint8Array} [encodedMask = null]
     *        If specified, the decoder will not read mask information from the input and use the specified encoded
     *        mask data instead. Mask header/data must not be present in the LERC byte stream in this case.
     * @config {number} [noDataValue = LercCode.defaultNoDataValue]
     *        Pixel value to use for masked pixels.
     * @config {ArrayBufferView|Array} [pixelType = Float32Array]
     *        The desired type of the pixelData array in the return value. Note that it is the caller's responsibility to
     *        provide an appropriate noDataValue if the default pixelType is overridden.
     * @config {boolean} [returnMask = false]
     *        If true, the return value will contain a maskData property of type Uint8Array which has one element per
     *        pixel, the value of which is 1 or 0 depending on whether that pixel's data is present or masked. If the
     *        input LERC data does not contain a mask, maskData will not be returned.
     * @config {boolean} [returnEncodedMask = false]
     *        If true, the return value will contain a encodedMaskData property, which can be passed into encode() as
     *        encodedMask.
     * @config {boolean} [returnFileInfo = false]
     *        If true, the return value will have a fileInfo property that contains metadata obtained from the
     *        LERC headers and the decoding process.
     * @config {boolean} [computeUsedBitDepths = false]
     *        If true, the fileInfo property in the return value will contain the set of all block bit depths
     *        encountered during decoding. Will only have an effect if returnFileInfo option is true.
     * @returns {{width, height, pixelData, minValue, maxValue, noDataValue, [maskData], [encodedMaskData], [fileInfo]}}
     */
    CntZImage.decode = function(input, options) {
      options = options || {};

      var skipMask = options.encodedMaskData || (options.encodedMaskData === null);
      var parsedData = parse(input, options.inputOffset || 0, skipMask);

      var noDataValue = (options.noDataValue !== null) ? options.noDataValue : CntZImage.defaultNoDataValue;

      var uncompressedData = uncompressPixelValues(parsedData, options.pixelType || Float32Array,
        options.encodedMaskData, noDataValue, options.returnMask);

      var result = {
        width: parsedData.width,
        height: parsedData.height,
        pixelData: uncompressedData.resultPixels,
        minValue: parsedData.pixels.minValue,
        maxValue: parsedData.pixels.maxValue,
        noDataValue: noDataValue
      };

      if (uncompressedData.resultMask) {
        result.maskData = uncompressedData.resultMask;
      }

      if (options.returnEncodedMask && parsedData.mask) {
        result.encodedMaskData = parsedData.mask.bitset ? parsedData.mask.bitset : null;
      }

      if (options.returnFileInfo) {
        result.fileInfo = formatFileInfo(parsedData);
        if (options.computeUsedBitDepths) {
          result.fileInfo.bitDepths = computeUsedBitDepths(parsedData);
        }
      }

      return result;
    };

    var uncompressPixelValues = function(data, TypedArrayClass, maskBitset, noDataValue, storeDecodedMask) {
      var blockIdx = 0;
      var numX = data.pixels.numBlocksX;
      var numY = data.pixels.numBlocksY;
      var blockWidth = Math.floor(data.width / numX);
      var blockHeight = Math.floor(data.height / numY);
      var scale = 2 * data.maxZError;
      maskBitset = maskBitset || ((data.mask) ? data.mask.bitset : null);

      var resultPixels, resultMask;
      resultPixels = new TypedArrayClass(data.width * data.height);
      if (storeDecodedMask && maskBitset) {
        resultMask = new Uint8Array(data.width * data.height);
      }
      var blockDataBuffer = new Float32Array(blockWidth * blockHeight);

      var xx, yy;
      for (var y = 0; y <= numY; y++) {
        var thisBlockHeight = (y !== numY) ? blockHeight : (data.height % numY);
        if (thisBlockHeight === 0) {
          continue;
        }
        for (var x = 0; x <= numX; x++) {
          var thisBlockWidth = (x !== numX) ? blockWidth : (data.width % numX);
          if (thisBlockWidth === 0) {
            continue;
          }

          var outPtr = y * data.width * blockHeight + x * blockWidth;
          var outStride = data.width - thisBlockWidth;

          var block = data.pixels.blocks[blockIdx];

          var blockData, blockPtr, constValue;
          if (block.encoding < 2) {
            // block is either uncompressed or bit-stuffed (encodings 0 and 1)
            if (block.encoding === 0) {
              // block is uncompressed
              blockData = block.rawData;
            } else {
              // block is bit-stuffed
              unstuff(block.stuffedData, block.bitsPerPixel, block.numValidPixels, block.offset, scale, blockDataBuffer, data.pixels.maxValue);
              blockData = blockDataBuffer;
            }
            blockPtr = 0;
          }
          else if (block.encoding === 2) {
            // block is all 0
            constValue = 0;
          }
          else {
            // block has constant value (encoding === 3)
            constValue = block.offset;
          }

          var maskByte;
          if (maskBitset) {
            for (yy = 0; yy < thisBlockHeight; yy++) {
              if (outPtr & 7) {
                //
                maskByte = maskBitset[outPtr >> 3];
                maskByte <<= outPtr & 7;
              }
              for (xx = 0; xx < thisBlockWidth; xx++) {
                if (!(outPtr & 7)) {
                  // read next byte from mask
                  maskByte = maskBitset[outPtr >> 3];
                }
                if (maskByte & 128) {
                  // pixel data present
                  if (resultMask) {
                    resultMask[outPtr] = 1;
                  }
                  resultPixels[outPtr++] = (block.encoding < 2) ? blockData[blockPtr++] : constValue;
                } else {
                  // pixel data not present
                  if (resultMask) {
                    resultMask[outPtr] = 0;
                  }
                  resultPixels[outPtr++] = noDataValue;
                }
                maskByte <<= 1;
              }
              outPtr += outStride;
            }
          } else {
            // mask not present, simply copy block over
            if (block.encoding < 2) {
              // duplicating this code block for performance reasons
              // blockData case:
              for (yy = 0; yy < thisBlockHeight; yy++) {
                for (xx = 0; xx < thisBlockWidth; xx++) {
                  resultPixels[outPtr++] = blockData[blockPtr++];
                }
                outPtr += outStride;
              }
            }
            else {
              // constValue case:
              for (yy = 0; yy < thisBlockHeight; yy++) {
                for (xx = 0; xx < thisBlockWidth; xx++) {
                  resultPixels[outPtr++] = constValue;
                }
                outPtr += outStride;
              }
            }
          }
          if ((block.encoding === 1) && (blockPtr !== block.numValidPixels)) {
            throw "Block and Mask do not match";
          }
          blockIdx++;
        }
      }

      return {
        resultPixels: resultPixels,
        resultMask: resultMask
      };
    };

    var formatFileInfo = function(data) {
      return {
        "fileIdentifierString": data.fileIdentifierString,
        "fileVersion": data.fileVersion,
        "imageType": data.imageType,
        "height": data.height,
        "width": data.width,
        "maxZError": data.maxZError,
        "eofOffset": data.eofOffset,
        "mask": data.mask ? {
          "numBlocksX": data.mask.numBlocksX,
          "numBlocksY": data.mask.numBlocksY,
          "numBytes": data.mask.numBytes,
          "maxValue": data.mask.maxValue
        } : null,
        "pixels": {
          "numBlocksX": data.pixels.numBlocksX,
          "numBlocksY": data.pixels.numBlocksY,
          "numBytes": data.pixels.numBytes,
          "maxValue": data.pixels.maxValue,
          "minValue": data.pixels.minValue,
          "noDataValue": data.noDataValue
        }
      };
    };

    var computeUsedBitDepths = function(data) {
      var numBlocks = data.pixels.numBlocksX * data.pixels.numBlocksY;
      var bitDepths = {};
      for (var i = 0; i < numBlocks; i++) {
        var block = data.pixels.blocks[i];
        if (block.encoding === 0) {
          bitDepths.float32 = true;
        } else if (block.encoding === 1) {
          bitDepths[block.bitsPerPixel] = true;
        } else {
          bitDepths[0] = true;
        }
      }

      return Object.keys(bitDepths);
    };

    var parse = function(input, fp, skipMask) {
      var data = {};

      // File header
      var fileIdView = new Uint8Array(input, fp, 10);
      data.fileIdentifierString = String.fromCharCode.apply(null, fileIdView);
      if (data.fileIdentifierString.trim() !== "CntZImage") {
        throw "Unexpected file identifier string: " + data.fileIdentifierString;
      }
      fp += 10;
      var view = new DataView(input, fp, 24);
      data.fileVersion = view.getInt32(0, true);
      data.imageType = view.getInt32(4, true);
      data.height = view.getUint32(8, true);
      data.width = view.getUint32(12, true);
      data.maxZError = view.getFloat64(16, true);
      fp += 24;

      // Mask Header
      if (!skipMask) {
        view = new DataView(input, fp, 16);
        data.mask = {};
        data.mask.numBlocksY = view.getUint32(0, true);
        data.mask.numBlocksX = view.getUint32(4, true);
        data.mask.numBytes = view.getUint32(8, true);
        data.mask.maxValue = view.getFloat32(12, true);
        fp += 16;

        // Mask Data
        if (data.mask.numBytes > 0) {
          var bitset = new Uint8Array(Math.ceil(data.width * data.height / 8));
          view = new DataView(input, fp, data.mask.numBytes);
          var cnt = view.getInt16(0, true);
          var ip = 2, op = 0;
          do {
            if (cnt > 0) {
              while (cnt--) { bitset[op++] = view.getUint8(ip++); }
            } else {
              var val = view.getUint8(ip++);
              cnt = -cnt;
              while (cnt--) { bitset[op++] = val; }
            }
            cnt = view.getInt16(ip, true);
            ip += 2;
          } while (ip < data.mask.numBytes);
          if ((cnt !== -32768) || (op < bitset.length)) {
            throw "Unexpected end of mask RLE encoding";
          }
          data.mask.bitset = bitset;
          fp += data.mask.numBytes;
        }
        else if ((data.mask.numBytes | data.mask.numBlocksY | data.mask.maxValue) == 0) {  // Special case, all nodata
          data.mask.bitset = new Uint8Array(Math.ceil(data.width * data.height / 8));
        }
      }

      // Pixel Header
      view = new DataView(input, fp, 16);
      data.pixels = {};
      data.pixels.numBlocksY = view.getUint32(0, true);
      data.pixels.numBlocksX = view.getUint32(4, true);
      data.pixels.numBytes = view.getUint32(8, true);
      data.pixels.maxValue = view.getFloat32(12, true);
      fp += 16;

      var numBlocksX = data.pixels.numBlocksX;
      var numBlocksY = data.pixels.numBlocksY;
      // the number of blocks specified in the header does not take into account the blocks at the end of
      // each row/column with a special width/height that make the image complete in case the width is not
      // evenly divisible by the number of blocks.
      var actualNumBlocksX = numBlocksX + ((data.width % numBlocksX) > 0 ? 1 : 0);
      var actualNumBlocksY = numBlocksY + ((data.height % numBlocksY) > 0 ? 1 : 0);
      data.pixels.blocks = new Array(actualNumBlocksX * actualNumBlocksY);
      var minValue = 1000000000;
      var blockI = 0;
      for (var blockY = 0; blockY < actualNumBlocksY; blockY++) {
        for (var blockX = 0; blockX < actualNumBlocksX; blockX++) {

          // Block
          var size = 0;
          var bytesLeft = input.byteLength - fp;
          view = new DataView(input, fp, Math.min(10, bytesLeft));
          var block = {};
          data.pixels.blocks[blockI++] = block;
          var headerByte = view.getUint8(0); size++;
          block.encoding = headerByte & 63;
          if (block.encoding > 3) {
            throw "Invalid block encoding (" + block.encoding + ")";
          }
          if (block.encoding === 2) {
            fp++;
            minValue = Math.min(minValue, 0);
            continue;
          }
          if ((headerByte !== 0) && (headerByte !== 2)) {
            headerByte >>= 6;
            block.offsetType = headerByte;
            if (headerByte === 2) {
              block.offset = view.getInt8(1); size++;
            } else if (headerByte === 1) {
              block.offset = view.getInt16(1, true); size += 2;
            } else if (headerByte === 0) {
              block.offset = view.getFloat32(1, true); size += 4;
            } else {
              throw "Invalid block offset type";
            }
            minValue = Math.min(block.offset, minValue);

            if (block.encoding === 1) {
              headerByte = view.getUint8(size); size++;
              block.bitsPerPixel = headerByte & 63;
              headerByte >>= 6;
              block.numValidPixelsType = headerByte;
              if (headerByte === 2) {
                block.numValidPixels = view.getUint8(size); size++;
              } else if (headerByte === 1) {
                block.numValidPixels = view.getUint16(size, true); size += 2;
              } else if (headerByte === 0) {
                block.numValidPixels = view.getUint32(size, true); size += 4;
              } else {
                throw "Invalid valid pixel count type";
              }
            }
          }
          fp += size;

          if (block.encoding === 3) {
            continue;
          }

          var arrayBuf, store8;
          if (block.encoding === 0) {
            var numPixels = (data.pixels.numBytes - 1) / 4;
            if (numPixels !== Math.floor(numPixels)) {
              throw "uncompressed block has invalid length";
            }
            arrayBuf = new ArrayBuffer(numPixels * 4);
            store8 = new Uint8Array(arrayBuf);
            store8.set(new Uint8Array(input, fp, numPixels * 4));
            var rawData = new Float32Array(arrayBuf);
            for (var j = 0; j < rawData.length; j++) {
              minValue = Math.min(minValue, rawData[j]);
            }
            block.rawData = rawData;
            fp += numPixels * 4;
          } else if (block.encoding === 1) {
            var dataBytes = Math.ceil(block.numValidPixels * block.bitsPerPixel / 8);
            var dataWords = Math.ceil(dataBytes / 4);
            arrayBuf = new ArrayBuffer(dataWords * 4);
            store8 = new Uint8Array(arrayBuf);
            store8.set(new Uint8Array(input, fp, dataBytes));
            block.stuffedData = new Uint32Array(arrayBuf);
            fp += dataBytes;
          }
        }
      }
      data.pixels.minValue = minValue;
      data.eofOffset = fp;
      return data;
    };

    var unstuff = function(src, bitsPerPixel, numPixels, offset, scale, dest, maxValue) {
      var bitMask = (1 << bitsPerPixel) - 1;
      var i = 0, o;
      var bitsLeft = 0;
      var n, buffer;
      var nmax = Math.ceil((maxValue - offset) / scale);
      // get rid of trailing bytes that are already part of next block
      var numInvalidTailBytes = src.length * 4 - Math.ceil(bitsPerPixel * numPixels / 8);
      src[src.length - 1] <<= 8 * numInvalidTailBytes;

      for (o = 0; o < numPixels; o++) {
        if (bitsLeft === 0) {
          buffer = src[i++];
          bitsLeft = 32;
        }
        if (bitsLeft >= bitsPerPixel) {
          n = (buffer >>> (bitsLeft - bitsPerPixel)) & bitMask;
          bitsLeft -= bitsPerPixel;
        } else {
          var missingBits = (bitsPerPixel - bitsLeft);
          n = ((buffer & bitMask) << missingBits) & bitMask;
          buffer = src[i++];
          bitsLeft = 32 - missingBits;
          n += (buffer >>> bitsLeft);
        }
        //pixel values may exceed max due to quantization
        dest[o] = n < nmax ? offset + n * scale : maxValue;
      }
      return dest;
    };

    return CntZImage;
  })();

  //version 2. Supports 2.1, 2.2, 2.3
  var Lerc2Codec = (function() {
    "use strict";
    // Note: currently, this module only has an implementation for decoding LERC data, not encoding. The name of
    // the class was chosen to be future proof, following LercCodec.

    /*****************************************
    * private static class bitsutffer used by Lerc2Codec
    *******************************************/
    var BitStuffer = {

      unstuff: function(block, dest, maxValue) {
        var src = block.stuffedData;
        var bitsPerPixel = block.bitsPerPixel;
        var numPixels = block.numValidPixel;
        var offset = block.offset;
        var scale = block.scale;
        var doLut = block.doLut;
        var lutArr = block.lutArr;

        var bitMask = (1 << bitsPerPixel) - 1;
        var i = 0, o;
        var bitsLeft = 0;
        var n, buffer, kk, missingBits;

        // get rid of trailing bytes that are already part of next block
        var numInvalidTailBytes = src.length * 4 - Math.ceil(bitsPerPixel * numPixels / 8);
        src[src.length - 1] <<= 8 * numInvalidTailBytes;
        if (doLut) {
          for (kk = 0; kk < lutArr.length; kk++) {
            lutArr[kk] = offset + lutArr[kk] * scale;
          }
          for (o = 0; o < numPixels; o++) {
            if (bitsLeft === 0) {
              buffer = src[i++];
              bitsLeft = 32;
            }
            if (bitsLeft >= bitsPerPixel) {
              n = (buffer >>> (bitsLeft - bitsPerPixel)) & bitMask;
              bitsLeft -= bitsPerPixel;
            }
            else {
              missingBits = (bitsPerPixel - bitsLeft);
              n = ((buffer & bitMask) << missingBits) & bitMask;
              buffer = src[i++];
              bitsLeft = 32 - missingBits;
              n += (buffer >>> bitsLeft);
            }
            dest[o] = lutArr[n];//offset + lutArr[n] * scale;
          }
        }
        else {
          var nmax = Math.ceil((maxValue - offset) / scale);
          for (o = 0; o < numPixels; o++) {
            if (bitsLeft === 0) {
              buffer = src[i++];
              bitsLeft = 32;
            }
            if (bitsLeft >= bitsPerPixel) {
              n = (buffer >>> (bitsLeft - bitsPerPixel)) & bitMask;
              bitsLeft -= bitsPerPixel;
            }
            else {
              missingBits = (bitsPerPixel - bitsLeft);
              n = ((buffer & bitMask) << missingBits) & bitMask;
              buffer = src[i++];
              bitsLeft = 32 - missingBits;
              n += (buffer >>> bitsLeft);
            }
            if (n < nmax) {
              dest[o] = offset + n * scale;
            }
            else {
              dest[o] = maxValue;
            }
          }
        }
        return dest;
      },

      unstuffLUT: function(src, bitsPerPixel, numPixels) {
        var bitMask = (1 << bitsPerPixel) - 1;
        var i = 0, o = 0, missingBits = 0, bitsLeft = 0, n = 0;
        var buffer;
        var dest = [];

        // get rid of trailing bytes that are already part of next block
        var numInvalidTailBytes = src.length * 4 - Math.ceil(bitsPerPixel * numPixels / 8);
        src[src.length - 1] <<= 8 * numInvalidTailBytes;

        for (o = 0; o < numPixels; o++) {
          if (bitsLeft === 0) {
            buffer = src[i++];
            bitsLeft = 32;
          }
          if (bitsLeft >= bitsPerPixel) {
            n = (buffer >>> (bitsLeft - bitsPerPixel)) & bitMask;
            bitsLeft -= bitsPerPixel;
          } else {
            missingBits = (bitsPerPixel - bitsLeft);
            n = ((buffer & bitMask) << missingBits) & bitMask;
            buffer = src[i++];
            bitsLeft = 32 - missingBits;
            n += (buffer >>> bitsLeft);
          }
          //dest[o] = offset + n * scale;
          dest.push(n);
        }
        return dest;
      },

      unstuff2: function(block, dest, maxValue) {
        var src = block.stuffedData;
        var bitsPerPixel = block.bitsPerPixel;
        var numPixels = block.numValidPixel;
        var offset = block.offset;
        var scale = block.scale;
        var doLut = block.doLut;
        var lutArr = block.lutArr;

        var bitMask = (1 << bitsPerPixel) - 1;
        var i = 0, o;
        var bitsLeft = 0, bitPos = 0;
        var n, buffer, kk, missingBits;
        if (doLut) {
          for (kk = 0; kk < lutArr.length; kk++) {
            lutArr[kk] = offset + lutArr[kk] * scale;
          }
          for (o = 0; o < numPixels; o++) {
            if (bitsLeft === 0) {
              buffer = src[i++];
              bitsLeft = 32;
              bitPos = 0;
            }
            if (bitsLeft >= bitsPerPixel) {
              n = ((buffer >>> bitPos) & bitMask);
              bitsLeft -= bitsPerPixel;
              bitPos += bitsPerPixel;
            } else {
              missingBits = (bitsPerPixel - bitsLeft);
              n = (buffer >>> bitPos) & bitMask;
              buffer = src[i++];
              bitsLeft = 32 - missingBits;
              n |= (buffer & ((1 << missingBits) - 1)) << (bitsPerPixel - missingBits);
              bitPos = missingBits;
            }
            dest[o] = lutArr[n];
          }
        }
        else {
          var nmax = Math.ceil((maxValue - offset) / scale);
          for (o = 0; o < numPixels; o++) {
            if (bitsLeft === 0) {
              buffer = src[i++];
              bitsLeft = 32;
              bitPos = 0;
            }
            if (bitsLeft >= bitsPerPixel) {
              //no unsigned left shift
              n = ((buffer >>> bitPos) & bitMask);
              bitsLeft -= bitsPerPixel;
              bitPos += bitsPerPixel;
            } else {
              missingBits = (bitsPerPixel - bitsLeft);
              n = (buffer >>> bitPos) & bitMask;//((buffer & bitMask) << missingBits) & bitMask;
              buffer = src[i++];
              bitsLeft = 32 - missingBits;
              n |= (buffer & ((1 << missingBits) - 1)) << (bitsPerPixel - missingBits);
              bitPos = missingBits;
            }
            if (n < nmax) {
              dest[o] = offset + n * scale;
            }
            else {
              dest[o] = maxValue;
            }
          }
        }
        return dest;
      },

      unstuffLUT2: function(src, bitsPerPixel, numPixels) {
        var bitMask = (1 << bitsPerPixel) - 1;
        var i = 0, o = 0, missingBits = 0, bitsLeft = 0, n = 0, bitPos = 0, bitsUnoccupied = 32 - bitsPerPixel;
        var buffer;
        var dest = [];

        // get rid of trailing bytes that are already part of next block
        var numInvalidTailBytes = src.length * 4 - Math.ceil(bitsPerPixel * numPixels / 8);
        src[src.length - 1] <<= 8 * numInvalidTailBytes;
        
        for (o = 0; o < numPixels; o++) {
          if (bitsLeft === 0) {
            buffer = src[i++];
            bitsLeft = 32;
            bitPos = 0;
          }
          if (bitsLeft >= bitsPerPixel) {
            //no unsigned left shift
            n = ((buffer >>> bitPos) & bitMask);
            bitsLeft -= bitsPerPixel;
            bitPos += bitsPerPixel;
          } else {
            missingBits = (bitsPerPixel - bitsLeft);
            n = (buffer >>> bitPos) & bitMask;//((buffer & bitMask) << missingBits) & bitMask;
            buffer = src[i++];
            bitsLeft = 32 - missingBits;
            n |= (buffer & ((1 << missingBits) - 1)) << (bitsPerPixel - missingBits);
            bitPos = missingBits;
          }
          dest.push(n);
        }
        return dest;
      }
    };

    /*****************************************
    *private static class used by Lerc2Codec
    ******************************************/
    var Lerc2Helpers = {
      HUFFMAN_LUT_BITS_MAX: 12, //use 2^12 lut
      HUFFMAN_BUILD_TREE: true, //false to use sparse array - bug?      
      computeChecksumFletcher32: function(input) {

        var sum1 = 0xffff, sum2 = 0xffff;
        var len = input.length;
        var words = Math.floor(len / 2);
        var i = 0;
        while (words) {
          var tlen = (words >= 359) ? 359 : words;
          words -= tlen;
          do {
            sum1 += (input[i++] << 8);
            sum2 += sum1 += input[i++];
          } while (--tlen);

          sum1 = (sum1 & 0xffff) + (sum1 >>> 16);
          sum2 = (sum2 & 0xffff) + (sum2 >>> 16);
        }

        // add the straggler byte if it exists
        if (len & 1)
          sum2 += sum1 += (input[i] << 8);

        // second reduction step to reduce sums to 16 bits
        sum1 = (sum1 & 0xffff) + (sum1 >>> 16);
        sum2 = (sum2 & 0xffff) + (sum2 >>> 16);

        return (sum2 << 16 | sum1) >>>0;
      },

      readHeaderInfo: function(input, data) {
        var ptr = data.ptr;
        var fileIdView = new Uint8Array(input, ptr, 6);
        var headerInfo = {};
        headerInfo.fileIdentifierString = String.fromCharCode.apply(null, fileIdView);
        if (headerInfo.fileIdentifierString.lastIndexOf("Lerc2", 0) !== 0) {
          throw "Unexpected file identifier string (expect Lerc2 ): " + headerInfo.fileIdentifierString;
        }
        ptr += 6;

        var view = new DataView(input, ptr, 52);
        headerInfo.fileVersion = view.getInt32(0, true);
        ptr += 4;
        if (headerInfo.fileVersion >= 3) {
          headerInfo.checksum = view.getUint32(4, true); //nrows
          ptr += 4;
        }
        view = new DataView(input, ptr, 48);

        headerInfo.height = view.getUint32(0, true); //nrows
        headerInfo.width = view.getUint32(4, true); //ncols
        headerInfo.numValidPixel = view.getUint32(8, true);
        headerInfo.microBlockSize = view.getInt32(12, true);
        headerInfo.blobSize = view.getInt32(16, true);
        headerInfo.imageType = view.getInt32(20, true);

        headerInfo.maxZError = view.getFloat64(24, true);
        headerInfo.zMin = view.getFloat64(32, true);
        headerInfo.zMax = view.getFloat64(40, true);
        ptr += 48;
        data.headerInfo = headerInfo;
        data.ptr = ptr;

        var checksum;
        if (headerInfo.fileVersion >= 3) {
          checksum = this.computeChecksumFletcher32(new Uint8Array(input, ptr - 48, headerInfo.blobSize - 14));
          if (checksum !== headerInfo.checksum) {
            throw "Checksum failed."
          }
        }
        return true;
      },

      readMask: function(input, data) {
        var ptr = data.ptr;
        var headerInfo = data.headerInfo;
        var numPixels = headerInfo.width * headerInfo.height;
        var numValidPixel = headerInfo.numValidPixel;

        var view = new DataView(input, ptr, 4);
        var mask = {};
        mask.numBytes = view.getUint32(0, true);
        ptr += 4;

        // Mask Data
        if ((0 === numValidPixel || numPixels === numValidPixel) && 0 !== mask.numBytes) {
          throw ("invalid mask");
        }
        var bitset, resultMask;
        if (numValidPixel === 0) {
          bitset = new Uint8Array(Math.ceil(numPixels / 8));
          mask.bitset = bitset;
          resultMask = new Uint8Array(numPixels);
          data.pixels.resultMask = resultMask;
          ptr += mask.numBytes;
        }// ????? else if (data.mask.numBytes > 0 && data.mask.numBytes< data.numValidPixel) {
        else if (mask.numBytes > 0) {
          bitset = new Uint8Array(Math.ceil(numPixels / 8));
          view = new DataView(input, ptr, mask.numBytes);
          var cnt = view.getInt16(0, true);
          var ip = 2, op = 0, val = 0;
          do {
            if (cnt > 0) {
              while (cnt--) { bitset[op++] = view.getUint8(ip++); }
            } else {
              val = view.getUint8(ip++);
              cnt = -cnt;
              while (cnt--) { bitset[op++] = val; }
            }
            cnt = view.getInt16(ip, true);
            ip += 2;
          } while (ip < mask.numBytes);
          if ((cnt !== -32768) || (op < bitset.length)) {
            throw "Unexpected end of mask RLE encoding";
          }

          resultMask = new Uint8Array(numPixels);
          var mb = 0, k = 0;

          for (k = 0; k < numPixels; k++) {
            if (k & 7) {
              mb = bitset[k >> 3];
              mb <<= k & 7;
            }
            else {
              mb = bitset[k >> 3];
            }
            if (mb & 128) {
              resultMask[k] = 1;
            }
          }
          data.pixels.resultMask = resultMask;

          mask.bitset = bitset;
          ptr += mask.numBytes;
        }
        data.ptr = ptr;
        data.mask = mask;
        return true;
      },

      readDataOneSweep: function(input, data, OutPixelTypeArray) {
        var ptr = data.ptr;
        var headerInfo = data.headerInfo;
        var numPixels = headerInfo.width * headerInfo.height;
        var imageType = headerInfo.imageType;
        var numBytes = headerInfo.numValidPixel * Lerc2Helpers.getDateTypeSize(imageType);
        //data.pixels.numBytes = numBytes;
        var rawData;
        if (OutPixelTypeArray === Uint8Array) {
          rawData = new Uint8Array(input, ptr, numBytes);
        }
        else {
          var arrayBuf = new ArrayBuffer(numBytes);
          var store8 = new Uint8Array(arrayBuf);
          store8.set(new Uint8Array(input, ptr, numBytes));
          rawData = new OutPixelTypeArray(arrayBuf);
        }
        if (rawData.length === numPixels) {
          data.pixels.resultPixels = rawData;
        }
        else  //mask
        {
          data.pixels.resultPixels = new OutPixelTypeArray(numPixels);
          var z = 0, k = 0;
          for (k = 0; k < numPixels; k++) {
            if (data.pixels.resultMask[k]) {
              data.pixels.resultPixels[k] = rawData[z++];
            }
          }
        }
        //console.debug("raw length " + rawData.length + " data height * width" + numPixels);

        ptr += numBytes;
        data.ptr = ptr;       //return data;
        return true;
      },

      readHuffman: function(input, data, OutPixelTypeArray) {
        var headerInfo = data.headerInfo;
        var numPixels = headerInfo.width * headerInfo.height;
        var BITS_MAX = this.HUFFMAN_LUT_BITS_MAX; //8 is slow for the large test image
        var useTree = this.HUFFMAN_BUILD_TREE;
        //console.log("use tree" + useTree);
        var size_max = 1 << BITS_MAX;
        /* ************************
         * reading code table
         *************************/
        var view = new DataView(input, data.ptr, 16);
        data.ptr += 16;
        var version = view.getInt32(0, true);
        if (version < 2) {
          throw "unsupported Huffman version";
        }
        var size = view.getInt32(4, true);
        var i0 = view.getInt32(8, true);
        var i1 = view.getInt32(12, true);
        //if (i0 >= i1 || size > size_max) {//we decode
        //  return false;
        //}
        if (i0 >= i1) {
          return false;
        }
        var blockDataBuffer = new Uint32Array(i1 - i0);
        Lerc2Helpers.decodeBits(input, data, blockDataBuffer, OutPixelTypeArray);
        var codeTable = []; //size
        var i, j, k, len;

        for (i = i0; i < i1; i++) {
          j = i - (i < size ? 0 : size);//wrap around
          codeTable[j] = { first: blockDataBuffer[i - i0], second: null };
        }

        var dataBytes = input.byteLength - data.ptr;
        var dataWords = Math.ceil(dataBytes / 4);
        var arrayBuf = new ArrayBuffer(dataWords * 4);
        var store8 = new Uint8Array(arrayBuf);
        store8.set(new Uint8Array(input, data.ptr, dataBytes));
        var stuffedData = new Uint32Array(arrayBuf);
        //var stuffedData = new Uint32Array(input,data.ptr,Math.floor((input.byteLength-data.ptr)/4));
        var bitPos = 0, word, srcPtr = 0;
        word = stuffedData[0];
        for (i = i0; i < i1; i++) {
          j = i - (i < size ? 0 : size);//wrap around
          len = codeTable[j].first;
          if (len > 0) {
            codeTable[j].second = (word << bitPos) >>> (32 - len);

            if (32 - bitPos >= len) {
              bitPos += len;
              if (bitPos === 32) {
                bitPos = 0;
                srcPtr++;
                word = stuffedData[srcPtr];
              }
            }
            else {
              bitPos += len - 32;
              srcPtr++;
              word = stuffedData[srcPtr];
              codeTable[j].second |= word >>> (32 - bitPos);
            }
          }
        }

        //this could be questionable ???
        //data.ptr = data.ptr + (srcPtr + 1) * 4 + (bitPos > 1 ? 4 : 0);
        data.ptr = data.ptr + (srcPtr) * 4 + (bitPos > 0 ? 4 : 0);
        //finished reading code table


        /* ************************
         * building lut
         *************************/
        var offset = data.headerInfo.imageType === 0 ? 128 : 0;
        var height = data.headerInfo.height;
        var width = data.headerInfo.width;
        var numBitsLUT = 0, numBitsLUTQick = 0;
        var tree = new TreeNode();
        for (i = 0; i < codeTable.length; i++) {
          if (codeTable[i] !== undefined) {
            numBitsLUT = Math.max(numBitsLUT, codeTable[i].first);
          }
        }
        if (numBitsLUT >= BITS_MAX) {
          numBitsLUTQick = BITS_MAX;
          console.log("WARning, large NUM LUT BITS IS " + numBitsLUT);
        }
        else {
          numBitsLUTQick = numBitsLUT;
        }

        var decodeLut = [], entry, code, numEntries, decodeLutSparse = [], jj, currentBit, node;
        for (i = i0; i < i1; i++) {
          j = i - (i < size ? 0 : size);//wrap around
          len = codeTable[j].first;
          if (len > 0) {
            entry = [len, j];
            if (len <= numBitsLUTQick) {
              code = codeTable[j].second << (numBitsLUTQick - len);
              numEntries = 1 << (numBitsLUTQick - len);
              for (k = 0; k < numEntries; k++) {
                decodeLut[code | k] = entry;
              }
            }
            else if (useTree) {
              //build tree
              code = codeTable[j].second;
              node = tree;
              if (len === 31) {
                console.log("warning length 31");
              }
              for (jj = len - 1; jj >= 0; jj--) {
                currentBit = code >>> jj & 1; //no left shift as length could be 30,31
                if (currentBit) {
                  if (!node.right) {
                    node.right = new TreeNode();
                  }
                  node = node.right;
                }
                else {
                  if (!node.left) {
                    node.left = new TreeNode();
                  }
                  node = node.left;
                }
                if (jj === 0 && !node.val) {
                  node.val = entry[1];
                }
              }
            }
            else { //numBitsLUT > size_max
              code = codeTable[j].second;// << (numBitsLUT - len);
              decodeLutSparse[code] = entry;
            }
          }
        }
        console.log(numBitsLUT);
        //console.log(decodeLutSparse);
        /* ************************
        *  decode
        *  *************************/
        var val, delta, mask = data.pixels.resultMask, valTmp, valTmpQuick, prevVal = 0, ii = 0, tempEntry;

        if (bitPos > 0) {
          srcPtr++;
          bitPos = 0;
        }
        word = stuffedData[srcPtr];

        var resultPixels = new OutPixelTypeArray(numPixels);
        if (data.headerInfo.numValidPixel === width * height) { //all valid
          for (k = 0, i = 0; i < height; i++) {
            for (j = 0; j < width; j++, k++) {
              //console.log("k "+k +" srcptr"+srcPtr +" word" + word);
              val = 0;
              valTmp = (word << bitPos) >>> (32 - numBitsLUTQick);
              valTmpQuick = valTmp;// >>> deltaBits;
              if (32 - bitPos < numBitsLUTQick) {
                valTmp |= ((stuffedData[srcPtr + 1]) >>> (64 - bitPos - numBitsLUTQick));
                valTmpQuick = valTmp;// >>> deltaBits;
              }
              if (decodeLut[valTmpQuick])    // if there, move the correct number of bits and done
              {
                val = decodeLut[valTmpQuick][1];
                bitPos += decodeLut[valTmpQuick][0];
                //console.log("val " + val + " bitPos" + bitPos);
              }
              else if (useTree) {
                valTmp = (word << bitPos) >>> (32 - numBitsLUT);
                valTmpQuick = valTmp;// >>> deltaBits;
                if (32 - bitPos < numBitsLUT) {
                  valTmp |= ((stuffedData[srcPtr + 1]) >>> (64 - bitPos - numBitsLUT));
                  valTmpQuick = valTmp;// >>> deltaBits;
                }
                node = tree;
                if (numBitsLUT === 31) {
                  console.log("warning length 31");
                }
                for (ii = 0; ii < numBitsLUT; ii++) {
                  //currentBit = (valTmp & (1 <<(numBitsLUT-ii-1)))>>> (numBitsLUT-ii-1);
                  currentBit = valTmp >>> (numBitsLUT - ii - 1) & 1;
                  node = currentBit ? node.right : node.left;
                  //console.log(node);
                  if (!(node.left || node.right)) {
                    val = node.val;
                    bitPos = bitPos + ii + 1;
                    break;
                  }
                }
              }
              else {
                valTmp = (word << bitPos) >>> (32 - numBitsLUT);
                valTmpQuick = valTmp;// >>> deltaBits;
                if (32 - bitPos < numBitsLUT) {
                  valTmp |= ((stuffedData[srcPtr + 1]) >>> (64 - bitPos - numBitsLUT));
                  valTmpQuick = valTmp;// >>> deltaBits;
                }
                for (ii = BITS_MAX + 1; ii <= numBitsLUT; ii++) {
                  tempEntry = decodeLutSparse[valTmp >>> (numBitsLUT - ii)];
                  if (tempEntry) {
                    val = tempEntry[1];
                    bitPos += tempEntry[0];
                    //bitPos += ii;
                    break;
                  }
                }
              }
              if (bitPos >= 32) {
                bitPos -= 32;
                srcPtr++;
                word = stuffedData[srcPtr];
              }

              delta = val - offset;

              if (j > 0)
                delta += prevVal;    // use overflow
              else if (i > 0)
                delta += resultPixels[k - width];
              else
                delta += prevVal;

              delta &= 0xFF; //overflow
              //console.log(delta + " " + k);
              resultPixels[k] = delta;//overflow
              prevVal = delta;
            }
          }
          //goes here
        }
        else { //not all valid, use mask
          for (k = 0, i = 0; i < height; i++) {
            for (j = 0; j < width; j++, k++) {
              if (mask[k]) {
                //console.log("k "+k +" srcptr"+srcPtr +" word" + word);
                val = 0;
                valTmp = (word << bitPos) >>> (32 - numBitsLUTQick);
                valTmpQuick = valTmp;// >>> deltaBits;
                if (32 - bitPos < numBitsLUTQick) {
                  valTmp |= ((stuffedData[srcPtr + 1]) >>> (64 - bitPos - numBitsLUTQick));
                  valTmpQuick = valTmp;// >>> deltaBits;
                }
                if (decodeLut[valTmpQuick])    // if there, move the correct number of bits and done
                {
                  val = decodeLut[valTmpQuick][1];
                  bitPos += decodeLut[valTmpQuick][0];
                  //console.log("val " + val + " bitPos" + bitPos);
                }
                else if (useTree) {
                  valTmp = (word << bitPos) >>> (32 - numBitsLUT);
                  valTmpQuick = valTmp;// >>> deltaBits;
                  if (32 - bitPos < numBitsLUT) {
                    valTmp |= ((stuffedData[srcPtr + 1]) >>> (64 - bitPos - numBitsLUT));
                    valTmpQuick = valTmp;// >>> deltaBits;
                  }
                  node = tree;
                  if (numBitsLUT === 31) {
                    console.log("warning length 31");
                  }
                  for (ii = 0; ii < numBitsLUT; ii++) {
                    currentBit = valTmp >>> (numBitsLUT - ii - 1) & 1;
                    node = currentBit ? node.right : node.left;
                    //console.log(node);
                    if (!(node.left || node.right)) {
                      val = node.val;
                      bitPos = bitPos + ii + 1;
                      break;
                    }
                  }
                }
                else {
                  valTmp = (word << bitPos) >>> (32 - numBitsLUT);
                  valTmpQuick = valTmp;// >>> deltaBits;
                  if (32 - bitPos < numBitsLUT) {
                    valTmp |= ((stuffedData[srcPtr + 1]) >>> (64 - bitPos - numBitsLUT));
                    valTmpQuick = valTmp;// >>> deltaBits;
                  }
                  for (ii = BITS_MAX + 1; ii <= numBitsLUT; ii++) {
                    tempEntry = decodeLutSparse[valTmp >>> (numBitsLUT - ii)];
                    if (tempEntry) {
                      val = tempEntry[1];
                      bitPos += tempEntry[0];
                      //bitPos += ii;
                      break;
                    }
                  }
                }
                if (bitPos >= 32) {
                  bitPos -= 32;
                  srcPtr++;
                  word = stuffedData[srcPtr];
                }

                delta = val - offset;

                if (j > 0 && mask[k - 1])
                  delta += prevVal;    // use overflow
                else if (i > 0 && mask[k - width])
                  delta += resultPixels[k - width];
                else
                  delta += prevVal;

                delta &= 0xFF; //overflow
                //console.log(delta + " " + k);               
                resultPixels[k] = delta;//overflow
                prevVal = delta;
              }
            }
          }
        }
        data.pixels.resultPixels = resultPixels;
        data.ptr = data.ptr + (srcPtr) * 4 + (bitPos > 1 ? 4 : 0);

      },

      decodeBits: function(input, data, blockDataBuffer, OutPixelTypeArray) {
        {
          //bitstuff encoding is 3
          var fileVersion = data.headerInfo.fileVersion;
          var block = {};
          block.ptr = 0;
          var view = new DataView(input, data.ptr, 5);//to do
          var headerByte = view.getUint8(0);
          block.ptr++;
          var bits67 = headerByte >> 6;
          var n = (bits67 === 0) ? 4 : 3 - bits67;
          block.doLut = (headerByte & 32) > 0 ? true : false;//5th bit
          var numBits = headerByte & 31;
          var numElements = 0;
          if (n === 1) {
            numElements = view.getUint8(block.ptr); block.ptr++;
          } else if (n === 2) {
            numElements = view.getUint16(block.ptr, true); block.ptr += 2;
          } else if (n === 4) {
            numElements = view.getUint32(block.ptr, true); block.ptr += 4;
          } else {
            throw "Invalid valid pixel count type";
          }
          block.numValidPixel = numElements;
          block.offset = 0;
          var scale = 2 * data.headerInfo.maxZError;
          var arrayBuf, store8, dataBytes, dataWords, lutArr;

          if (block.doLut) {
            //console.log("lut");
            data.counter.lut++;
            block.lutBytes = view.getUint8(block.ptr);
            block.lutBitsPerElement = numBits;
            block.ptr++;
            dataBytes = Math.ceil((block.lutBytes - 1) * numBits / 8);
            dataWords = Math.ceil(dataBytes / 4);
            arrayBuf = new ArrayBuffer(dataWords * 4);
            store8 = new Uint8Array(arrayBuf);

            data.ptr += block.ptr;
            store8.set(new Uint8Array(input, data.ptr, dataBytes));

            block.lutData = new Uint32Array(arrayBuf);
            data.ptr += dataBytes;

            block.bitsPerPixel = 0;
            while ((block.lutBytes - 1) >> block.bitsPerPixel) {
              block.bitsPerPixel++;
            }
            dataBytes = Math.ceil(numElements * block.bitsPerPixel / 8);
            dataWords = Math.ceil(dataBytes / 4);
            arrayBuf = new ArrayBuffer(dataWords * 4);
            store8 = new Uint8Array(arrayBuf);
            store8.set(new Uint8Array(input, data.ptr, dataBytes));
            block.stuffedData = new Uint32Array(arrayBuf);
            data.ptr += dataBytes;
            if (fileVersion >= 3) {
              lutArr = BitStuffer.unstuffLUT2(block.lutData, block.lutBitsPerElement, block.lutBytes - 1);
            }
            else {
              lutArr = BitStuffer.unstuffLUT(block.lutData, block.lutBitsPerElement, block.lutBytes - 1);
            }
            lutArr.unshift(0);
            block.scale = scale;
            block.lutArr = lutArr;
            if (fileVersion >= 3) {
              BitStuffer.unstuff2(block, blockDataBuffer, data.headerInfo.zMax);
            }
            else {
              BitStuffer.unstuff(block, blockDataBuffer, data.headerInfo.zMax);
            }
          }
          else {
            //console.debug("bitstuffer");
            data.counter.bitstuffer++;
            block.bitsPerPixel = numBits;
            block.scale = scale;
            data.ptr += block.ptr;
            if (block.bitsPerPixel > 0) {

              dataBytes = Math.ceil(block.numValidPixel * block.bitsPerPixel / 8);
              dataWords = Math.ceil(dataBytes / 4);
              arrayBuf = new ArrayBuffer(dataWords * 4);
              store8 = new Uint8Array(arrayBuf);
              store8.set(new Uint8Array(input, data.ptr, dataBytes));
              block.stuffedData = new Uint32Array(arrayBuf);
              data.ptr += dataBytes;
              if (fileVersion >= 3) {
                BitStuffer.unstuff2(block, blockDataBuffer, data.headerInfo.zMax);
              }
              else {
                BitStuffer.unstuff(block, blockDataBuffer, data.headerInfo.zMax);
              }
            }
          }
        }

      },

      readTiles: function(input, data, OutPixelTypeArray) {
        var headerInfo = data.headerInfo;
        var width = headerInfo.width;
        var height = headerInfo.height;
        var fileVersion = headerInfo.fileVersion;
        var microBlockSize = headerInfo.microBlockSize;
        var imageType = headerInfo.imageType;
        var scale = 2 * headerInfo.maxZError;
        var numBlocksX = Math.ceil(width / microBlockSize);
        var numBlocksY = Math.ceil(height / microBlockSize);
        data.pixels.numBlocksY = numBlocksY;
        data.pixels.numBlocksX = numBlocksX;
        data.pixels.ptr = 0;
        var row = 0, col = 0, blockY = 0, blockX = 0, thisBlockHeight = 0, thisBlockWidth = 0, bytesLeft = 0, headerByte = 0, bits67 = 0, testCode = 0, outPtr = 0, outStride = 0, numBytes = 0, bytesleft = 0, z = 0, n = 0, numBits = 0, numElements = 0, blockPtr = 0;
        //console.debug("num block x y" + numBlocksX.toString() + " " + numBlocksY.toString());
        var view, block, arrayBuf, store8, rawData, dataBytes, dataWords, lutArr;
        //var blockDataBuffer = new Float32Array(microBlockSize * microBlockSize);
        var blockDataBuffer = new OutPixelTypeArray(microBlockSize * microBlockSize);
        var lastBlockHeight = (height % microBlockSize) || microBlockSize;
        var lastBlockWidth = (width % microBlockSize) || microBlockSize;

        for (blockY = 0; blockY < numBlocksY; blockY++) {
          thisBlockHeight = (blockY !== numBlocksY - 1) ? microBlockSize : lastBlockHeight;
          for (blockX = 0; blockX < numBlocksX; blockX++) {            
            //console.debug("y" + blockY + " x" + blockX);
            thisBlockWidth = (blockX !== numBlocksX - 1) ? microBlockSize : lastBlockWidth;

            outPtr = blockY * width * microBlockSize + blockX * microBlockSize;
            outStride = width - thisBlockWidth;

            bytesLeft = input.byteLength - data.ptr;
            view = new DataView(input, data.ptr, Math.min(10, bytesLeft));
            block = {};
            block.ptr = 0;
            headerByte = view.getUint8(0);
            block.ptr++;
            bits67 = (headerByte >> 6) & 0xFF;
            testCode = (headerByte >> 2) & 15;    // use bits 2345 for integrity check
            if (testCode !== (((blockX * microBlockSize) >> 3) & 15)) {
              throw "integrity issue";
              //return false;
            }

            block.encoding = headerByte & 3;
            if (block.encoding > 3) {
              data.ptr += block.ptr;
              throw "Invalid block encoding (" + block.encoding + ")";
            }
            else if (block.encoding === 2) { //constant 0                    
              data.counter.constant++;
              data.ptr += block.ptr;
              continue;
            }
            else if (block.encoding === 0) {  //uncompressed
              data.counter.uncompressed++;
              data.ptr += block.ptr;
              numBytes = thisBlockHeight * thisBlockWidth * Lerc2Helpers.getDateTypeSize(imageType);
              bytesleft = input.byteLength - data.ptr;
              numBytes = numBytes < bytesleft ? numBytes : bytesleft;
              arrayBuf = new ArrayBuffer(numBytes);
              store8 = new Uint8Array(arrayBuf);
              store8.set(new Uint8Array(input, data.ptr, numBytes));
              rawData = new OutPixelTypeArray(arrayBuf);
              z = 0;
              if (data.pixels.resultMask) {
                for (row = 0; row < thisBlockHeight; row++) {
                  for (col = 0; col < thisBlockWidth; col++) {
                    if (data.pixels.resultMask[outPtr]) {
                      data.pixels.resultPixels[outPtr] = rawData[z++];
                    }
                    outPtr++;
                  }
                  outPtr += outStride;
                }
              }
              else {//all valid
                for (row = 0; row < thisBlockHeight; row++) {
                  for (col = 0; col < thisBlockWidth; col++) {
                    data.pixels.resultPixels[outPtr++] = rawData[z++];
                  }
                  outPtr += outStride;
                }
              }
              data.ptr += z * Lerc2Helpers.getDateTypeSize(imageType);
              //if (rawData.length !== z * Lerc2Helpers.getDateTypeSize(imageType))
              //console.debug("raw data length,  please investigate");
            }
            else { //1 or 3
              block.offsetType = Lerc2Helpers.getDataTypeUsed(imageType, bits67);
              block.offset = Lerc2Helpers.getOnePixel(block, view);
              if (block.encoding === 3) //constant offset value
              {
                data.ptr += block.ptr;
                data.counter.constantoffset++;
                //you can delete the following resultMask case in favor of performance because val is constant and users use nodata mask, otherwise nodatavalue post processing handles it too.
                if (data.pixels.resultMask) {
                  for (row = 0; row < thisBlockHeight; row++) {
                    for (col = 0; col < thisBlockWidth; col++) {
                      if (data.pixels.resultMask[outPtr]) {
                        data.pixels.resultPixels[outPtr] = block.offset;
                      }
                      outPtr++;
                    }
                    outPtr += outStride;
                  }
                }
                else {
                  for (row = 0; row < thisBlockHeight; row++) {
                    for (col = 0; col < thisBlockWidth; col++) {
                      data.pixels.resultPixels[outPtr++] = block.offset;
                    }
                    outPtr += outStride;
                  }
                }
              }
              else { //bitstuff encoding is 3
                headerByte = view.getUint8(block.ptr); block.ptr++;
                bits67 = headerByte >> 6;
                n = (bits67 === 0) ? 4 : 3 - bits67;
                block.doLut = (headerByte & 32) > 0 ? true : false;//5th bit
                numBits = headerByte & 31;
                numElements = 0;
                if (n === 1) {
                  numElements = view.getUint8(block.ptr); block.ptr++;
                } else if (n === 2) {
                  numElements = view.getUint16(block.ptr, true); block.ptr += 2;
                } else if (n === 4) {
                  numElements = view.getUint32(block.ptr, true); block.ptr += 4;
                } else {
                  throw "Invalid valid pixel count type";
                }
                block.numValidPixel = numElements;

                if (block.doLut) {
                  //console.log("lut");
                  data.counter.lut++;
                  block.lutBytes = view.getUint8(block.ptr);
                  block.lutBitsPerElement = numBits;
                  block.ptr++;
                  dataBytes = Math.ceil((block.lutBytes - 1) * numBits / 8);
                  dataWords = Math.ceil(dataBytes / 4);
                  arrayBuf = new ArrayBuffer(dataWords * 4);
                  store8 = new Uint8Array(arrayBuf);

                  data.ptr += block.ptr;
                  store8.set(new Uint8Array(input, data.ptr, dataBytes));

                  block.lutData = new Uint32Array(arrayBuf);
                  data.ptr += dataBytes;

                  block.bitsPerPixel = 0;
                  while ((block.lutBytes - 1) >> block.bitsPerPixel) {
                    block.bitsPerPixel++;
                  }
                  dataBytes = Math.ceil(numElements * block.bitsPerPixel / 8);
                  dataWords = Math.ceil(dataBytes / 4);
                  arrayBuf = new ArrayBuffer(dataWords * 4);
                  store8 = new Uint8Array(arrayBuf);
                  store8.set(new Uint8Array(input, data.ptr, dataBytes));
                  block.stuffedData = new Uint32Array(arrayBuf);
                  data.ptr += dataBytes;
                  if (fileVersion >= 3) {
                    lutArr = BitStuffer.unstuffLUT2(block.lutData, block.lutBitsPerElement, block.lutBytes - 1);
                  }
                  else {
                    lutArr = BitStuffer.unstuffLUT(block.lutData, block.lutBitsPerElement, block.lutBytes - 1);
                  }
                  lutArr.unshift(0);
                  block.scale = scale;
                  block.lutArr = lutArr;
                  if (fileVersion >= 3) {
                    BitStuffer.unstuff2(block, blockDataBuffer, data.headerInfo.zMax);
                  }
                  else {
                    BitStuffer.unstuff(block, blockDataBuffer, data.headerInfo.zMax);
                  }
                }
                else {
                  data.counter.bitstuffer++;
                  block.bitsPerPixel = numBits;
                  block.scale = scale;
                  data.ptr += block.ptr;
                  if (block.bitsPerPixel > 0) {

                    dataBytes = Math.ceil(block.numValidPixel * block.bitsPerPixel / 8);
                    dataWords = Math.ceil(dataBytes / 4);
                    arrayBuf = new ArrayBuffer(dataWords * 4);
                    store8 = new Uint8Array(arrayBuf);
                    store8.set(new Uint8Array(input, data.ptr, dataBytes));
                    block.stuffedData = new Uint32Array(arrayBuf);
                    data.ptr += dataBytes;
                    if (fileVersion >= 3) {
                      BitStuffer.unstuff2(block, blockDataBuffer, data.headerInfo.zMax);
                    }
                    else {
                      BitStuffer.unstuff(block, blockDataBuffer, data.headerInfo.zMax);
                    }
                  }
                }
                blockPtr = 0;
                if (data.pixels.resultMask) {
                  for (row = 0; row < thisBlockHeight; row++) {
                    for (col = 0; col < thisBlockWidth; col++) {
                      if (data.pixels.resultMask[outPtr]) {
                        data.pixels.resultPixels[outPtr] = blockDataBuffer[blockPtr++];
                      }
                      outPtr++;
                    }
                    outPtr += outStride;
                  }
                }
                else {
                  for (row = 0; row < thisBlockHeight; row++) {
                    for (col = 0; col < thisBlockWidth; col++) {                      
                      data.pixels.resultPixels[outPtr++] = blockDataBuffer[blockPtr++];
                    }
                    outPtr += outStride;
                  }
                }
              }
            }
          }
        }
      },

      /*****************
      *  private methods (helper methods)
      *****************/

      formatFileInfo: function(data) {
        return {
          "fileIdentifierString": data.headerInfo.fileIdentifierString,
          "fileVersion": data.headerInfo.fileVersion,
          "imageType": data.headerInfo.imageType,
          "height": data.headerInfo.height,
          "width": data.headerInfo.width,
          "numValidPixel": data.headerInfo.numValidPixel,
          "microBlockSize": data.headerInfo.microBlockSize,
          "blobSize": data.headerInfo.blobSize,
          "maxZError": data.headerInfo.maxZError,
          "pixelType": Lerc2Helpers.getPixelType(data.headerInfo.imageType),
          "eofOffset": data.eofOffset,
          "mask": data.mask ? {
            "numBytes": data.mask.numBytes
          } : null,
          "pixels": {
            "numBlocksX": data.pixels.numBlocksX,
            "numBlocksY": data.pixels.numBlocksY,
            //"numBytes": data.pixels.numBytes,
            "maxValue": data.headerInfo.zMax,
            "minValue": data.headerInfo.zMin,
            "noDataValue": data.noDataValue
          }
        };
      },

      constructConstantSurface: function(data) {
        var val = data.headerInfo.zMax;
        var numPixels = data.headerInfo.height * data.headerInfo.width;
        var k = 0;
        if (data.pixels.resultMask) {
          for (k = 0; k < numPixels; k++) {
            if (data.pixels.resultMask[k]) {
              data.pixels.resultPixels[k] = val;
            }
          }
        }
        else {
          for (k = 0; k < numPixels; k++) {
            data.pixels.resultPixels[k] = val;
          }
        }
        return;
      },

      getDataTypeArray: function(t) {
        var tp;
        switch (t) {
          case 0: //char
            tp = Int8Array;
            break;
          case 1: //byte
            tp = Uint8Array;
            break;
          case 2: //short
            tp = Int16Array;
            break;
          case 3: //ushort
            tp = Uint16Array;
            break;
          case 4:
            tp = Int32Array;
            break;
          case 5:
            tp = Uint32Array;
            break;
          case 6:
            tp = Float32Array;
            break;
          case 7:
            tp = Float64Array;
            break;
          default:
            tp = Float32Array;
        }
        return tp;
      },

      getPixelType: function(t) {
        var tp;
        switch (t) {
          case 0: //char
            tp = "S8";
            break;
          case 1: //byte
            tp = "U8";
            break;
          case 2: //short
            tp = "S16";
            break;
          case 3: //ushort
            tp = "U16";
            break;
          case 4:
            tp = "S32";
            break;
          case 5:
            tp = "U32";
            break;
          case 6:
            tp = "F32";
            break;
          case 7:
            tp = "F64"; //not supported
            break;
          default:
            tp = "F32";
        }
        return tp;
      },

      isValidPixelValue: function(t, val) {
        if (val === null || val === undefined) {
          return false;
        }
        var isValid;
        switch (t) {
          case 0: //char
            isValid = val >= -128 && val <= 127;
            break;
          case 1: //byte  (unsigned char)
            isValid = val >= 0 && val <= 255;
            break;
          case 2: //short
            isValid = val >= -32768 && val <= 32767;
            break;
          case 3: //ushort
            isValid = val >= 0 && val <= 65536;
            break;
          case 4: //int 32
            isValid = val >= -2147483648 && val <= 2147483647;
            break;
          case 5: //uinit 32
            isValid = val >= 0 && val <= 4294967296;
            break;
          case 6:
            isValid = val >= -3.4027999387901484e+38 && val <= 3.4027999387901484e+38;
            break;
          case 7:
            isValid = val >= 5e-324 && val <= 1.7976931348623157e+308;
            break;
          default:
            isValid = false;
        }
        return isValid;
      },

      getDateTypeSize: function(t) {
        var s = 0;
        switch (t) {
          case 0: //ubyte
          case 1: //byte
            s = 1;
            break;
          case 2: //short
          case 3: //ushort
            s = 2;
            break;
          case 4:
          case 5:
          case 6:
            s = 4;
            break;
          case 7:
            s = 8;
            break;
          default:
            s = t;
        }
        return s;
      },

      getDataTypeUsed: function(dt, tc) {
        var t = dt;
        switch (dt) {
          case 2: //short
          case 4: //long
            t = dt - tc;
            break;
          case 3: //ushort
          case 5: //ulong
            t = dt - 2 * tc;
            break;
          case 6: //float
            if (0 === tc) {
              t = dt;
            }
            else if (1 === tc) {
              t = 2;
            }
            else {
              t = 1;//byte
            }
            break;
          case 7: //double
            if (0 === tc) {
              t = dt;
            }
            else {
              t = dt - 2 * tc + 1;
            }
            break;
          default:
            t = dt;
            break;
        }
        return t;
      },

      getOnePixel: function(block, view) {
        var temp = 0;
        switch (block.offsetType) {
          case 0: //char
            temp = view.getInt8(block.ptr);
            block.ptr++;
            break;
          case 1: //byte
            temp = view.getUint8(block.ptr);
            block.ptr++;
            break;
          case 2:
            temp = view.getInt16(block.ptr, true);
            block.ptr += 2;
            break;
          case 3:
            temp = view.getUint16(block.ptr, true);
            block.ptr += 2;
            break;
          case 4:
            temp = view.getInt32(block.ptr, true);
            block.ptr += 4;
            break;
          case 5:
            temp = view.getUInt32(block.ptr, true);
            block.ptr += 4;
            break;
          case 6:
            temp = view.getFloat32(block.ptr, true);
            block.ptr += 4;
            break;
          case 7:
            //temp = view.getFloat64(block.ptr, true);
            //block.ptr += 8;
            //lerc2 encoding doesnt handle float 64, force to float32???
            temp = view.getFloat64(block.ptr, true);
            block.ptr += 8;
            break;
          default:
            throw ("the decoder does not understand this pixel type");
        }
        return temp;
      }
    };

    /***************************************************
    *private class for a tree node. Huffman code is in Lerc2Helpers
    ****************************************************/
    var TreeNode = function(val, left, right) {
      this.val = val;
      this.left = left;
      this.right = right;
    };

    var Lerc2Codec = {
      /**
       * Decode a LERC2 byte stream and return an object containing the pixel data and some required and optional
       * information about it, such as the image's width and height.
       *
       * @param {ArrayBuffer} input The LERC input byte stream
       * @param {object} [options] Decoding options, containing any of the following properties:
       * @config {number} [inputOffset = 0]
       *        Skip the first inputOffset bytes of the input byte stream. A valid LERC file is expected at that position.
       * @config {boolean} [returnFileInfo = false]
       *        If true, the return value will have a fileInfo property that contains metadata obtained from the
       *        LERC headers and the decoding process.
       * @config {boolean} [buildHuffmanTree = false]
       *        If true, the Huffman encoded data will be resolved through Huffman tree instead of sparse arrays for if tree depth>=12.
       *
       * ********removed options compared to LERC1. We can bring some of them back if needed. 
       * removed pixel type. LERC2 is typed and doesn't require user to give pixel type
       * changed encodedMaskData to maskData. LERC2 's js version make it faster to use maskData directly.
       * removed returnMask. mask is used by LERC2 internally and is cost free. In case of user input mask, it's returned as well and has neglible cost.
       * removed nodatavalue. Because LERC2 pixels are typed, nodatavalue will sacrify a useful value for many types (8bit, 16bit) etc,
       *       user has to be knowledgable enough about raster and their data to avoid usability issues. so nodata value is simply removed now. 
       *       We can add it back later if their's a clear requirement.
       * removed encodedMask. This option was not implemented in LercCodec. It can be done after decoding (less efficient)
       * removed computeUsedBitDepths.
       *
       *
       * response changes compared to LERC1
       * 1. encodedMaskData is not available
       * 2. noDataValue is optional (returns only if user's noDataValue is with in the valid data type range)
       * 3. maskData is always available
       * @returns {{width, height, pixelData, minValue, maxValue, maskData, [fileInfo]}}
       */


      /*****************
      *  public properties
      ******************/
      //HUFFMAN_LUT_BITS_MAX: 12, //use 2^12 lut, not configurable
      //HUFFMAN_BUILD_TREE: false, //use sparse array or tree

      /*****************
      *  public methods
      *****************/
      decode: function(/*byte array*/ input, /*object*/ options) {
        //currently there's a bug in the sparse array, so please do not set to false
        Lerc2Helpers.HUFFMAN_BUILD_TREE = options.buildHuffmanTree !== false;
        options = options || {};
        var skipMask = options.maskData || (options.maskData === null);
        var noDataValue = options.noDataValue;

        //initialize
        var i = 0, data = {};
        data.ptr = options.inputOffset || 0;
        data.pixels = {};

        // File header
        if (!Lerc2Helpers.readHeaderInfo(input, data)) {
          return;
        }
        var headerInfo = data.headerInfo;
        var OutPixelTypeArray = Lerc2Helpers.getDataTypeArray(headerInfo.imageType);

        // Mask Header
        if (skipMask) {
          data.pixels.resultMask = options.maskData;
          data.ptr += 4;
        }
        else {
          if (!Lerc2Helpers.readMask(input, data)) {
            //invalid mask
            return;
          }
        }

        var numPixels = headerInfo.width * headerInfo.height;
        data.pixels.resultPixels = new OutPixelTypeArray(numPixels);

        data.counter = {
          onesweep: 0,
          uncompressed: 0,
          lut: 0,
          bitstuffer: 0,
          constant: 0,
          constantoffset: 0
        };
        if (headerInfo.numValidPixel !== 0) {
          //not tested
          if (headerInfo.zMax === headerInfo.zMin) //constant surface
          {
            Lerc2Helpers.constructConstantSurface(data);
          }
          else {
            var view = new DataView(input, data.ptr, 2);
            var bReadDataOneSweep = view.getUint8(0, true);
            data.ptr++;
            if (bReadDataOneSweep) {
              //console.debug("OneSweep");
              Lerc2Helpers.readDataOneSweep(input, data, OutPixelTypeArray);
            }
            else {
              //lerc2.1: //bitstuffing + lut
              //lerc2.2: //bitstuffing + lut + huffman 
              //lerc2.3: new bitstuffer
              if (headerInfo.fileVersion > 1 && headerInfo.imageType <= 1 && Math.abs(headerInfo.maxZError - 0.5) < 0.00001) {
                var bReadHuffman = view.getUint8(1, true);
                data.ptr++;
                if (bReadHuffman) {//1
                  console.log("Huffman");
                  Lerc2Helpers.readHuffman(input, data, OutPixelTypeArray);
                }
                else {
                  Lerc2Helpers.readTiles(input, data, OutPixelTypeArray);
                }
              }
              else {
                Lerc2Helpers.readTiles(input, data, OutPixelTypeArray);
              }
            }
          }
        }

        data.eofOffset = data.ptr;
        var diff;
        if (options.inputOffset) {
          diff = data.headerInfo.blobSize + options.inputOffset - data.ptr;
          if (Math.abs(diff) >= 1) {
            //console.debug("incorrect eof: dataptr " + data.ptr + " offset " + options.inputOffset + " blobsize " + data.headerInfo.blobSize + " diff: " + diff);
            data.eofOffset = options.inputOffset + data.headerInfo.blobSize;
          }
        }
        else {
          diff = data.headerInfo.blobSize - data.ptr;
          if (Math.abs(diff) >= 1) {
            //console.debug("incorrect first band eof: dataptr " + data.ptr + " blobsize " + data.headerInfo.blobSize + " diff: " + diff);
            data.eofOffset = data.headerInfo.blobSize;
          }
        }

        var result = {
          width: headerInfo.width,
          height: headerInfo.height,
          pixelData: data.pixels.resultPixels,
          minValue: headerInfo.zMin,
          maxValue: headerInfo.zMax,
          maskData: data.pixels.resultMask
          //noDataValue: noDataValue
        };

        //we should remove this if there's no existing client
        //optional noDataValue processing, it's user's responsiblity
        if (data.pixels.resultMask && Lerc2Helpers.isValidPixelValue(headerInfo.imageType, noDataValue)) {
          var mask = data.pixels.resultMask;
          for (i = 0; i < numPixels; i++) {
            if (!mask[i]) {
              result.pixelData[i] = noDataValue;
            }
          }
          result.noDataValue = noDataValue;
        }
        data.noDataValue = noDataValue;
        if (options.returnFileInfo) {
          result.fileInfo = Lerc2Helpers.formatFileInfo(data);
        }
        return result;
      },

      getBandCount: function(/*byte array*/ input) {
        var count = 0;
        var i = 0;
        var temp = {};
        temp.ptr = 0;
        temp.pixels = {};
        while (i < input.byteLength - 58) {
          Lerc2Helpers.readHeaderInfo(input, temp);
          i += temp.headerInfo.blobSize;
          count++;
          temp.ptr = i;
        }
        return count;
      }
    };

    return Lerc2Codec;
  })();


  /************wrapper**********************************************
   * This is a wrapper on top of the basic lerc stream decoding
   *      used to decode multi band pixel blocks for various pixel types.
   * decodeLercRaster(xhr.response)
   * decodeLercRaster(xhr.response, {pixelType:"U8"}); leave pixelType out in favor of F32 
   * ***********************************************************/
  var Lerc = {
    decode: function(encodedData, options) {
      options = options || {};
      var fileIdView = new Uint8Array(encodedData, 0, 10);
      var fileIdentifierString = String.fromCharCode.apply(null, fileIdView);
      var lerc;
      if (fileIdentifierString.trim() === "CntZImage") {
        lerc = LercCodec;
      }
      else if (fileIdentifierString.substring(0, 5) === "Lerc2") {
        lerc = Lerc2Codec;
      }
      else {
        throw "Unexpected file identifier string: " + fileIdentifierString;
      }

      var iPlane = 0, inputOffset = 0, eof = encodedData.byteLength - 10, encodedMaskData, maskData;
      var decodedPixelBlock = {
        width: 0,
        height: 0,
        pixels: [],
        pixelType: options.pixelType,
        mask: null,
        statistics: []
      };

      while (inputOffset < eof) {
        var result = lerc.decode(encodedData, {
          inputOffset: inputOffset,//for both lerc1 and lerc2
          encodedMaskData: encodedMaskData,//lerc1 only
          maskData: maskData,//lerc2 only
          returnMask: iPlane === 0 ? true : false,//lerc1 only
          returnEncodedMask: iPlane === 0 ? true : false,//lerc1 only
          returnFileInfo: true,//for both lerc1 and lerc2
          pixelType: options.pixelType || null,//lerc1 only
          noDataValue: options.noDataValue || null,//lerc1 only
          buildHuffmanTree: options.buildHuffmanTree
        });

        inputOffset = result.fileInfo.eofOffset;
        if (iPlane === 0) {
          encodedMaskData = result.encodedMaskData;//lerc1
          maskData = result.maskData;//lerc2
          decodedPixelBlock.width = result.width;
          decodedPixelBlock.height = result.height;
          decodedPixelBlock.pixelType = result.pixelType || result.fileInfo.pixelType;
          decodedPixelBlock.mask = result.maskData;
        };

        iPlane++;
        decodedPixelBlock.pixels.push(result.pixelData),
        decodedPixelBlock.statistics.push({
          minValue: result.minValue,
          maxValue: result.maxValue,
          noDataValue: result.noDataValue
        });
      }
      return decodedPixelBlock;
    }
  };

  //amd loader such as dojo and requireJS
  if (typeof define === 'function' && define.amd) {
    define([], function() { return Lerc; });
  }
  else {//assign to this, most likely window
    this.Lerc = Lerc;
  }
})();