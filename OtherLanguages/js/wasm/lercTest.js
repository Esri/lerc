import { load, getBandCount, decode } from "./LercDecode.js";

function decodeTest() {
  const t000 =
    "76,101,114,99,50,32,4,0,0,0,1,18,203,197,0,1,0,0,0,1,0,0,1,0,0,0,131,123,0,0,8,0,0,0,210,7,0,0,1,0,0,0,0,0,0,0,0,0,224,63,0,0,0,0,0,192,87,64,0,0,0,0,0,192,87,64,140,7,0,0,235,255,0,1,0,64,225,255,0,1,0,96,225,255,0,1,0,96,245,255,0,1,0,31,237,255,255,1,0,224,245,255,0,1,0,15,237,255,255,1,0,224,245,255,0,1,0,7,237,255,255,1,0,224,245,255,0,1,0,7,237,255,255,1,0,254,245,255,0,1,0,7,236,255,255,1,0,126,246,255,0,1,0,7,235,255,255,247,255,0,2,0,192,3,235,255,255,1,0,64,248,255,0,2,0,240,3,235,255,255,2,0,240,128,249,255,0,2,0,127,227,234,255,255,1,0,252,249,255,0,2,0,127,243,233,255,255,249,255,0,2,0,63,251,234,255,255,1,0,252,249,255,0,2,0,63,231,234,255,255,1,0,224,249,255,0,2,0,63,243,234,255,255,1,0,128,249,255,0,2,0,63,187,234,255,255,248,255,0,2,0,31,251,235,255,255,1,0,254,248,255,0,2,0,31,215,235,255,255,1,0,252,248,255,0,2,0,31,207,235,255,255,3,0,248,0,12,250,255,0,1,0,31,234,255,255,3,0,240,0,30,250,255,0,1,0,15,234,255,255,4,0,225,128,63,224,251,255,0,1,0,15,234,255,255,10,0,199,131,255,240,0,64,0,0,0,7,233,255,255,9,0,199,255,240,31,192,0,0,0,7,231,255,255,7,0,254,63,192,0,0,0,15,229,255,255,5,0,254,0,0,0,0,229,255,255,5,0,254,0,0,0,3,228,255,255,4,0,0,0,0,7,230,255,255,6,0,254,55,0,0,0,7,230,255,255,6,0,224,16,0,0,0,7,230,255,255,6,0,64,0,0,0,0,7,231,255,255,7,0,252,0,60,0,0,0,7,231,255,255,7,0,248,0,62,0,0,0,7,231,255,255,7,0,248,0,31,192,0,0,7,231,255,255,7,0,240,0,255,224,0,0,7,231,255,255,7,0,240,0,255,240,0,0,15,231,255,255,7,0,224,6,255,224,0,0,15,231,255,255,7,0,200,14,255,240,0,0,15,231,255,255,7,0,152,15,255,240,0,0,15,231,255,255,7,0,176,31,255,240,0,0,15,231,255,255,7,0,240,31,255,240,0,0,15,231,255,255,7,0,240,31,255,240,0,0,15,231,255,255,7,0,240,31,255,224,0,0,15,231,255,255,7,0,224,63,255,192,0,0,15,231,255,255,7,0,224,63,255,142,0,0,31,231,255,255,7,0,224,63,255,15,0,0,31,231,255,255,7,0,224,63,255,31,0,0,31,231,255,255,7,0,224,63,255,255,0,0,31,231,255,255,7,0,192,63,255,255,0,0,63,231,255,255,7,0,192,63,255,255,0,0,63,231,255,255,7,0,192,31,255,255,0,0,63,231,255,255,7,0,192,31,255,255,128,0,63,231,255,255,7,0,192,31,255,255,128,0,127,231,255,255,7,0,192,31,255,255,128,0,63,231,255,255,7,0,192,31,255,255,128,0,63,231,255,255,7,0,192,31,255,254,0,0,63,231,255,255,7,0,192,31,255,252,0,0,63,231,255,255,7,0,192,31,255,248,0,0,63,231,255,255,7,0,224,63,255,248,0,1,31,231,255,255,7,0,224,127,255,248,0,7,31,231,255,255,7,0,224,127,255,240,0,63,31,231,255,255,7,0,240,255,255,224,0,255,15,231,255,255,7,0,251,255,255,240,3,255,15,228,255,255,4,0,254,7,255,15,225,255,255,1,0,15,225,255,255,1,0,15,225,255,255,1,0,15,225,255,255,1,0,31,225,255,255,1,0,31,225,255,255,1,0,31,225,255,255,1,0,63,225,255,255,1,0,63,225,255,255,1,0,31,225,255,255,1,0,15,225,255,255,1,0,7,225,255,255,1,0,7,225,255,255,1,0,3,225,255,255,1,0,3,225,255,255,1,0,3,225,255,255,1,0,3,225,255,255,1,0,3,225,255,255,1,0,3,225,255,255,1,0,1,225,255,255,1,0,1,225,255,255,1,0,0,225,255,255,2,0,0,127,226,255,255,2,0,0,63,226,255,255,2,0,0,63,226,255,255,2,0,0,27,226,255,255,2,0,0,11,226,255,255,2,0,0,5,226,255,255,2,0,0,5,226,255,255,2,0,0,7,226,255,255,2,0,0,3,226,255,255,2,0,0,3,226,255,255,2,0,0,3,226,255,255,2,0,0,1,226,255,255,3,0,0,0,127,227,255,255,3,0,0,0,127,227,255,255,2,0,0,0,226,255,255,2,0,0,0,226,255,255,3,0,0,0,127,227,255,255,3,0,0,0,63,227,255,255,3,0,0,0,31,227,255,255,3,0,0,0,31,227,255,255,3,0,0,0,15,227,255,255,3,0,0,0,7,227,255,255,3,0,0,0,3,227,255,255,3,0,0,0,3,227,255,255,3,0,0,0,0,227,255,255,3,0,0,0,1,227,255,255,3,0,0,0,0,227,255,255,3,0,0,0,0,227,255,255,3,0,0,0,0,227,255,255,4,0,0,0,0,1,228,255,255,4,0,0,0,0,0,228,255,255,5,0,0,0,0,0,15,229,255,255,5,0,0,0,0,0,15,229,255,255,5,0,0,0,0,0,11,229,255,255,5,0,0,0,0,0,1,229,255,255,251,255,0,1,0,127,230,255,255,251,255,0,1,0,63,230,255,255,251,255,0,1,0,63,230,255,255,251,255,0,1,0,31,230,255,255,251,255,0,1,0,31,231,255,255,1,0,254,251,255,0,3,0,31,255,127,233,255,255,1,0,254,251,255,0,3,0,24,0,127,233,255,255,1,0,240,249,255,0,1,0,63,233,255,255,1,0,240,249,255,0,1,0,7,233,255,255,1,0,192,248,255,0,233,255,255,1,0,192,248,255,0,1,0,31,234,255,255,1,0,128,248,255,0,1,0,7,234,255,255,1,0,128,247,255,0,5,0,255,255,252,0,127,239,255,255,246,255,0,5,0,31,255,252,0,127,239,255,255,246,255,0,5,0,3,255,252,0,31,240,255,255,1,0,254,242,255,0,1,0,15,240,255,255,1,0,254,242,255,0,1,0,7,240,255,255,1,0,252,242,255,0,1,0,3,240,255,255,1,0,252,241,255,0,240,255,255,1,0,252,241,255,0,1,0,127,245,255,255,5,0,191,255,255,255,252,241,255,0,1,0,127,245,255,255,5,0,189,143,255,255,252,241,255,0,1,0,63,247,255,255,7,0,243,192,16,15,255,255,254,241,255,0,1,0,63,247,255,255,7,0,226,0,0,1,255,255,254,241,255,0,1,0,63,247,255,255,7,0,254,128,0,1,252,127,254,241,255,0,2,0,31,254,251,255,255,10,0,254,255,255,255,128,0,0,240,63,254,241,255,0,17,0,31,252,7,255,255,255,255,159,7,199,255,0,0,0,128,31,255,241,255,0,17,0,15,248,3,255,255,255,255,200,0,3,255,0,0,0,0,31,255,241,255,0,17,0,7,248,1,255,255,255,255,192,0,1,253,128,0,0,0,7,255,241,255,0,18,0,1,248,0,255,255,255,255,128,0,0,72,96,0,0,0,7,255,128,241,255,0,17,0,96,0,127,255,255,255,0,0,0,0,32,0,0,0,1,255,128,239,255,0,4,0,127,255,255,255,247,255,0,2,0,255,192,239,255,0,4,0,63,255,255,252,247,255,0,2,0,255,192,239,255,0,4,0,63,255,255,64,247,255,0,2,0,255,192,239,255,0,4,0,31,255,255,128,247,255,0,2,0,255,192,239,255,0,3,0,31,255,254,247,255,0,3,0,1,255,224,239,255,0,3,0,15,255,250,247,255,0,3,0,1,255,224,239,255,0,3,0,7,255,252,246,255,0,2,0,255,240,239,255,0,3,0,3,255,224,247,255,0,3,0,1,255,240,239,255,0,3,0,3,255,248,246,255,0,2,0,255,240,238,255,0,2,0,255,240,246,255,0,2,0,255,248,238,255,0,2,0,255,240,246,255,0,2,0,255,248,238,255,0,2,0,255,224,246,255,0,2,0,127,252,238,255,0,2,0,255,224,246,255,0,2,0,127,252,238,255,0,2,0,127,224,246,255,0,2,0,95,252,238,255,0,2,0,127,224,246,255,0,2,0,31,252,238,255,0,2,0,63,224,246,255,0,2,0,31,252,238,255,0,2,0,63,224,246,255,0,2,0,15,252,238,255,0,2,0,15,240,246,255,0,2,0,15,252,238,255,0,2,0,3,240,246,255,0,2,0,7,252,237,255,0,1,0,24,246,255,0,2,0,7,252,225,255,0,1,0,252,225,255,0,1,0,248,225,255,0,1,0,248,225,255,0,1,0,120,225,255,0,1,0,224,0,246,0,0,128"
      .split(",")
      .map((x) => Number(x));
  const t000U8 = new Uint8Array(t000);

  console.log(decode(t000U8));
}

function demo() {
  // decodeTest();
  // http://wju2.esri.com/data/lerc/testuv_w30_h20_float.lerc2: decode hr is 1, incorrect mask
  var defaultUrl =
    "http://wju2.esri.com/data/lerc/test_huffman_byte_dim3.lerc25";
  //http://sampleserver6.arcgisonline.com/arcgis/rest/services/Toronto/ImageServer/exportImage?bbox=-8844874.0651,5401062.402699997,-8828990.0651,5420947.402699997&bboxSR=&size=&imageSR=&time=&format=lerc&pixelType=UNKNOWN&noData=&noDataInterpretation=esriNoDataMatchAny&interpolation= RSP_BilinearInterpolation&compression=&compressionQuality=&bandIds=&mosaicRule=&renderingRule=&f=image
  var url =
    location.search.indexOf("?url=") === 0
      ? location.search.slice(5)
      : defaultUrl;
  var showNext,
    autoShowInterval = 3000;
  var list;
  document.getElementById("mapUrl").addEventListener("change", showTile());
  if (url.indexOf(".txt") > 0) {
    var listXhr = new XMLHttpRequest();
    listXhr.open("Get", url, true);
    listXhr.responseType = "text";
    listXhr.send();
    listXhr.onload = function () {
      list = listXhr.response.split("\r\n");
      currentIndex = -1;
      document.getElementById("divControls").style.display = "block";
      showTile(1);
    };
    listXhr.onerror = function () {
      url = defaultUrl;
      document.getElementById("mapUrl").value = url;
      showTile();
    };
  } else {
    document.getElementById("mapUrl").value = url;
    showTile();
  }
  function autoShow(btn) {
    if (showNext) {
      clearTimeout(showNext);
      showNext = null;
      btn.innerText = "Auto";
    } else {
      btn.innerText = "Pause";
      autoShowNext();
    }
  }

  function autoShowNext() {
    showNext = setTimeout(function () {
      showTile(1);
      if (currentIndex < list.length - 1 && showNext) {
        autoShowNext();
      }
    }, autoShowInterval);
  }

  function showTile(next) {
    if (next === -1) {
      if (currentIndex > 0) {
        document.getElementById("mapUrl").value = list[--currentIndex];
      }
    } else if (next === 1) {
      if (currentIndex < list.length - 1) {
        document.getElementById("mapUrl").value = list[++currentIndex];
      }
    }
    if (list) {
      document.getElementById("currentIndex").innerText =
        currentIndex + 1 + "/" + list.length;
    }
    //UI elements
    var div_Info = document.getElementById("imageInfo");
    var div_CurrentPv = document.getElementById("currentPV");
    var imageCanvas = document.getElementById("imageCanvas");
    var canvasPos = imageCanvas.getBoundingClientRect();
    var context = imageCanvas.getContext("2d");
    var mapUrl = document.getElementById("mapUrl").value || defaultUrl;
    //div_Header.innerHTML = "test";
    //if it fails you'll see the red
    context.clearRect(0, 0, imageCanvas.width, imageCanvas.height);
    context.fillStyle = "red";
    context.fillRect(0, 0, 100, 100);
    //fetching lerc
    var xhr = new XMLHttpRequest();
    xhr.open("Get", mapUrl, true);
    xhr.responseType = "arraybuffer";
    xhr.send();
    var pixels, mask, min, max, factor, planes, height, width;
    xhr.onreadystatechange = function () {
      if (xhr.readyState == 4 && xhr.status == 200) {
        var t0 = new Date();
        for (let i=0; i<100; i++) {
          Lerc.getBandCount(xhr.response);
        }
        var t1 = new Date();
        console.log("js getBandCount:" + (t1-t0)/1000);
        t0 = new Date();
        for (let i=0; i<100; i++) {
          getBandCount(xhr.response);
        }
        var t1 = new Date();
        console.log("wasm getBandCount:" + (t1-t0)/1000);
        t0 = new Date();
        for (let i=0; i<10; i++) {
        var decodedPixelBlock1 = Lerc.decode(xhr.response);
        }
        var t1 = new Date();
        console.log("js decode:" + (t1-t0)/1000);
        t0 = new Date();
        for (let i=0; i<10; i++) {
        var decodedPixelBlock = decode(xhr.response);
        }
        t1 = new Date();
        console.log("wasm decode:" + (t1-t0)/1000);
        var duration = new Date() - t0;
        width = decodedPixelBlock.width;
        height = decodedPixelBlock.height;
        pixels = decodedPixelBlock.pixels;
        mask = decodedPixelBlock.mask;
        imageCanvas.width = width;
        imageCanvas.height = height;

        var imageData = context.createImageData(width, height);
        var data = imageData.data;
        var pv = 0;
        var index = 0;
        var nPixels = width * height;
        planes = pixels.length;

        if (planes === 1) {
          min = decodedPixelBlock.statistics[0].minValue;
          max = decodedPixelBlock.statistics[0].maxValue;
          factor = 255 / (max - min);
          //optimization for no mask case  (mid of most rasters)
          if (mask) {
            for (var i = 0; i < nPixels; i++) {
              if (mask[i]) {
                pv = (pixels[0][i] - min) * factor;
                data[i * 4] = pv;
                data[i * 4 + 1] = pv;
                data[i * 4 + 2] = pv;
                data[i * 4 + 3] = 255;
              } else {
                data[i * 4 + 3] = 0;
              }
            }
          } else {
            for (var i = 0; i < nPixels; i++) {
              pv = (pixels[0][i] - min) * factor;
              data[i * 4] = pv;
              data[i * 4 + 1] = pv;
              data[i * 4 + 2] = pv;
              data[i * 4 + 3] = 255;
            }
          }
        } else if (planes >= 3) {
          //show first 3 bands here, use same min/max for demonstration purpose.
          min = Math.min.apply(
            null,
            decodedPixelBlock.statistics.map(function (x) {
              return x.minValue;
            })
          );
          max = Math.max.apply(
            null,
            decodedPixelBlock.statistics.map(function (x) {
              return x.maxValue;
            })
          );
          factor = 255 / (max - min);
          if (mask) {
            for (var i = 0; i < nPixels; i++) {
              if (mask[i]) {
                data[i * 4] = (pixels[0][i] - min) * factor;
                data[i * 4 + 1] = (pixels[1][i] - min) * factor;
                data[i * 4 + 2] = (pixels[2][i] - min) * factor;
                data[i * 4 + 3] = 255;
              } else {
                data[i * 4 + 3] = 0;
              }
            }
          } else {
            for (var i = 0; i < nPixels; i++) {
              data[i * 4] = (pixels[0][i] - min) * factor;
              data[i * 4 + 1] = (pixels[1][i] - min) * factor;
              data[i * 4 + 2] = (pixels[2][i] - min) * factor;
              data[i * 4 + 3] = 255;
            }
          }
        }
        context.putImageData(imageData, 0, 0);
        div_Info.innerHTML =
          "max: " +
          max +
          ", min: " +
          min +
          ", width: " +
          width +
          ", height: " +
          height +
          ", planes: " +
          planes +
          ", dims: " +
          decodedPixelBlock.dimCount +
          ", decode time:" +
          duration;
      }
      imageCanvas.onmousemove = function (evt) {
        if (!pixels) {
          return;
        }
        var i = evt.clientX - canvasPos.left; //col
        var j = evt.clientY - canvasPos.top; //row
        var pos = j * width + i;
        if ((mask && mask[pos]) || !mask) {
          var pv = [];
          for (var kk = 0; kk < planes; kk++) {
            pv.push(pixels[kk][pos]);
          }
          div_CurrentPv.innerHTML = "current pixel value: " + pv.join(", ");
        } else {
          div_CurrentPv.innerHTML = "current pixel value: no data";
        }
      };
    };
  }
}

load().then(() => demo());

// onesweep
// testall_w1922_h1083_ushort.lerc2
// js getBandCount:0.006
// lercTest.js:112 wasm getBandCount:0.042
// lercTest.js:118 js decode:0.265
// lercTest.js:124 wasm decode:0.457

// charlottelas_256_256.lerc2
// read tiles lerc
// js getBandCount:0.018
// lercTest.js:112 wasm getBandCount:0.005
// lercTest.js:118 js decode:0.042
// lercTest.js:124 wasm decode:0.008

// wv2_512_512.lerc2
// js getBandCount:0.002
// lercTest.js:112 wasm getBandCount:0.095
// lercTest.js:118 js decode:0.173
// lercTest.js:124 wasm decode:0.1

// toronto_4band_16bit_bandmask2.lerc2
// js getBandCount:0.145
// lercTest.js:112 wasm getBandCount:0.138
// lercTest.js:118 js decode:0.675
// lercTest.js:124 wasm decode:0.664

// netcdf_3slices.lerc24
// js getBandCount:0.017
// lercTest.js:112 wasm getBandCount:0.005
// lercTest.js:118 js decode:0.071
// lercTest.js:124 wasm decode:0.017

// landsat_6band_8bit.lerc24
// js getBandCount:0.627
// lercTest.js:112 wasm getBandCount:0.599
// lercTest.js:118 js decode:4.068
// lercTest.js:124 wasm decode:1.25