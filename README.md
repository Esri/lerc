# LERC - Limited Error Raster Compression

## What is LERC?

LERC is an Open Source format which supports rapid encoding/decoding with *dynamic* image compression and preserves the precision of the original raster.

This repository contains both a C++ library for encoding/decoding images and JavaScript and Python codecs for decoding LERC files.

## The LERC API

Function | Description
--- | ---
`bool ComputeBufferSize(...)` | Computes the buffer size that needs to be allocated so the image can be Lerc compressed into that buffer. The size is accurate to the byte. This function is optional. It is faster than `Encode(...)`. It can also be called to decide whether an image or image tile should be encoded by Lerc or another method.
`bool Encode(...)` | Compresses a given image into a pre-allocated buffer. If that buffer is too small, the function fails and returns `false`. The function returns the number of bytes written.
`bool GetLercInfo(...)` | Looks into a given Lerc byte blob and returns a struct with all the header info. From this, the image to be decoded can be allocated and constructed. This function is optional. You don't need to call it if you already know the image properties such as tile size and data type.
`bool Decode(...)` | Uncompresses a given Lerc byte blob into a pre-allocated image. If the data found in the Lerc byte blob does not fit the specified image properties, the function fails and returns `false`.


To better support the case that not all image pixels are valid, the BitMask class is provided. It permits easy and efficient checking and setting pixels to valid / invalid.

See the sample program src/LercTest/main.cpp how the above functions and classes are called and used. Also see the two header files in the include/ folder and the comments in there.

One more comment about multiple bands. You can either store each band into its own Lerc byte blob which allows you to access / decode each band individually. Lerc also allows to stack bands together into one single Lerc byte blob. This can be useful if the bands are always used together anyway.

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

## How to use

For illustration, we have put together a simple test sample.

- build the Lerc.dll in src/Lerc. Copy the resulting Lerc.lib into the lib/ folder.
- build the test app in src/LercTest.
- copy the Lerc.dll next to LercTest.exe.
- run it.

## Lerc Properties

- works on any common data type, not just 8 bit:
  char, byte, short, ushort, int, uint, float, double.

- works with any given MaxZError or max coding error per pixel.

- can work with a bit mask that specifies which pixels are valid
  and which ones are not.

- is very fast: encoding time is about 20 ms per MegaPixel per band,
  decoding time is about 5 ms per MegaPixel per band.

- compression is better than most other compression methods for
  larger bitdepth data (int types larger than 8 bit, float, double).  

- for 8 bit data lossless compression, PNG can be better, but is
  much slower.

- in general for lossy compression with MaxZError > 0, the larger
  the error allowed, the stronger the compression.
  Compression factors larger than 100x have been reported.

- this Lerc package can read all (legacy) versions of Lerc, such as
  Lerc1, Lerc2 v1, and the current Lerc2 v2. It always writes the
  latest stable version.

The main principle of Lerc and history can be found in [doc/MORE.md](doc/MORE.md)

## Bugs?

The codecs Lerc2 and Lerc1 have been in use for years, bugs in those
low level modules are very unlikely. The top level layer that wraps
the different Lerc versions is newer. So if this package shows a bug,
it is most likely in that layer.

## Contact

Send an email to  <a href="mailto:tmaurer@esri.com">tmaurer@esri.com</a>

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
