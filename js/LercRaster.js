(function (exports) {
  /************usage**********************************************
   * This is a wrapper on top of the basic lerc stream decoding
   *      used to decode multi band pixel blocks for various pixel types.
   * load LercCodec.js first
   * decodeLercRaster(xhr.response)
   * decodeLercRaster(xhr.response, {pixelType:"U8"}); leave pixelType out in favor of F32 
   * ***********************************************************/
  var LercRaster = {
    decode: function (encodedData, options) {
      var iPlane = 0, inputOffset = 0, eof = encodedData.byteLength - 10, encodedMaskData;
      var decodedPixelBlock = {
        width: 0,
        height: 0,
        pixels: [],
        pixelType: options ? options.pixelType : null,
        mask: null,
        statistics: []
      };
      var lerc = LERC();
      while (inputOffset < eof) {
        var result = lerc.decode(encodedData, {
          inputOffset: inputOffset,
          encodedMaskData: encodedMaskData,
          returnMask: iPlane === 0 ? true : false,
          returnEncodedMask: iPlane === 0 ? true : false,
          returnFileInfo: true,
          pixelType: options ? options.pixelType : null,
          noDataValue: options ? options.noDataValue : null,
        });

        inputOffset = result.fileInfo.eofOffset;
        if (iPlane === 0) {
          encodedMaskData = result.encodedMaskData;
          decodedPixelBlock.width = result.width;
          decodedPixelBlock.height = result.height;
          decodedPixelBlock.pixelType = result.pixelType;
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

  if (exports) {
    exports.LercRaster = LercRaster;
  }
  else {
    this.LercRaster = LercRaster;
  }
})();