[![npm version][npm-img]][npm-url]

[npm-img]: https://img.shields.io/npm/v/lerc.svg?style=flat-square
[npm-url]: https://www.npmjs.com/package/lerc

# Lerc JS

> Rapid decoding for any pixel type, not just rgb or byte

## Usage

```js
npm install 'lerc'
```
```js
var Lerc = require('lerc');

Lerc.decode(xhr.response, { returnFileInfo: true });
```

## API Reference

<a name="module_Lerc"></a>

## Lerc
a module for decoding LERC blobs

<a name="exp_module_Lerc--decode"></a>

### decode(input, [options]) ⇒ <code>Object</code> ⏏
Decode a LERC2 byte stream and return an object containing the pixel data and optional metadata.

**Kind**: Exported function

| Param | Type | Description |
| --- | --- | --- |
| input | <code>ArrayBuffer</code> | The LERC input byte stream |
| [options] | <code>object</code> | options Decoding options |
| [options.inputOffset] | <code>number</code> | The number of bytes to skip in the input byte stream. A valid LERC file is expected at that position |
| [options.returnFileInfo] | <code>boolean</code> | If true, the return value will have a fileInfo property that contains metadata obtained from the LERC headers and the decoding process |

**Result Object Properties**

| Name | Type | Description |
| --- | --- | --- |
| width | <code>number</code> | Width of decoded image |
| height | <code>number</code> | Height of decoded image |
| pixelData | <code>object</code> | The actual decoded image |
| minValue | <code>number</code> | Minimum pixel value detected in decoded image |
| maxValue | <code>number</code> | Maximum pixel value detected in decoded image |
| maskData | <code>maskData</code> |  |
| fileInfo | <code>object</code> |  |

* * *

## Licensing

Copyright 2017 Esri

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and limitations under the License.

A local copy of the license and additional notices are located with the source distribution at:

http://github.com/Esri/lerc/

[](Esri Tags: raster, image, encoding, encoded, decoding, decoded, compression, codec, lerc)
[](Esri Language: JS, JavaScript, Python)
