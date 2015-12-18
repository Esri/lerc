# LERC - Limited Error Raster Compression


## When to use

In image or raster compression, there are 2 different ways to go:

- compress an image as much as possible but so it still looks ok
  (jpeg and relatives). The max coding error per pixel can be large.

- need control over the max coding error per pixel (elevation, 
  scientific data, medical image data, ...).
  Because of that, such data is often compressed using lossless methods,
  such as LZW, gzip, and the like. The compression ratios achieved
  are often low. On top the encoding is often slow and time consuming. 

Lerc allows you to set the max coding error per pixel allowed, called 
"MaxZError". You can specify any number from 0 (lossless) to a number
so large that the decoded image may come out flat. 

In a nutshell, if jpeg is good enough for your images, use jpeg. If not,
if you would use png instead, or gzip, then you may want to try out Lerc. 


## Lerc Properties

- works on any common data type, not just 8 bit:
  char, byte, short, ushort, int, uint, float, double. 

- works with any given MaxZError or max coding error per pixel. 

- can work with a bit mask that specifies which pixels are valid
  and which ones are not. 

- is very fast: encoding time is about 20 ms per MegaPixel per band,
  decoding time is about 5 ms per MegaPixel per band. 

- compression is better than most other compression methods for 
  larger bitdepth data (>= 16 bit, float, double). 

- for 8 bit data lossless compression, PNG can be better, but is
  much slower. 

- in general for lossy compression with MaxZError > 0, the larger
  the error allowed, the stronger the compression. 

- this Lerc package can read all (legacy) versions of Lerc, such as
  Lerc(1), Lerc2 v1, and the current Lerc2 v2. It always writes 
  the current version Lerc2 v2.


## How to use

- build the solution in src/Lerc. Copy the resulting Lerc.lib to lib/.
- build the test app in src/LercTest. 
- run it. 

- the main include file is include/Lerc.h. See the comments there 
  about how to call the Lerc encode and decode functions. 

- packages such as MRF Lerc encode image data per band. This way 
  they can easily access each band individually. Lerc allows to
  stack bands together into a single Lerc blob. This can be useful 
  if the bands are always used together anyway. 

- you can find more information [here](https://github.com/Esri/lerc/tree/master/doc).

## About bugs

The codecs Lerc2 and Lerc1 have been in use for years, bugs in those
low level modules are very unlikely. The top level layer that wraps 
the different Lerc versions is new. So if this package shows a bug,
it is most likely in that layer. 


## Licensing

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

A local copy of the license and additional notices are located with the
source distribution at:

http://github.com/Esri/lerc/

