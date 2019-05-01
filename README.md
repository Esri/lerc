# LERC - Limited Error Raster Compression

## What is LERC?

LERC is an open-source image or raster format which supports rapid encoding and decoding for any pixel type (not just RGB or Byte). Users set the maximum compression error per pixel while encoding, so the precision of the original input image is preserved (within user defined error bounds).

This repository contains both a C++ library for encoding/decoding images and JavaScript, C#, and Python decoders.

## The LERC C API

Function | Description
--- | ---
`uint lerc_computeCompressedSize(...)` | Computes the buffer size that needs to be allocated so the image can be Lerc compressed into that buffer. The size is accurate to the byte. This function is optional. It is faster than `lerc_encode(...)`. It can also be called to decide whether an image or image tile should be encoded by Lerc or another method.
`uint lerc_encode(...)` | Compresses a given image into a pre-allocated buffer. If that buffer is too small, the function fails with the corresponding error code. The function also returns the number of bytes written.
`uint lerc_getBlobInfo(...)` | Looks into a given Lerc byte blob and returns an array with all the header info. From this, the image to be decoded can be allocated and constructed. This function is optional. You don't need to call it if you already know the image properties such as tile size and data type.
`uint lerc_decode(...)` | Uncompresses a given Lerc byte blob into a pre-allocated image. If the data found in the Lerc byte blob does not fit the specified image properties, the function fails with the corresponding error code.
`uint lerc_decodeToDouble(...)` | Uncompresses a given Lerc byte blob into a pre-allocated image of type double independent of the compressed data type. This function was added mainly to be called from other languages such as C# and Python.

To support the case that not all image pixels are valid, a mask image can be passed. It has one byte per pixel, 1 for valid, 0 for invalid.

See the sample program `src/LercTest/main.cpp` which demonstrates how the above functions are called and used. Also see the two header files in the `include/` folder and the comments in there.

About multiple bands, or multiple values per pixel. This has changed with Lerc version 2.4. Before, you could either store each band into its own Lerc byte blob which allowed you to access / decode each band individually. Lerc also allowed to stack bands together into one single Lerc byte blob. This could be useful if the bands are always used together anyway. Now, since Lerc version 2.4, you can additionally store multiple values per pixel interleaved, meaning an array of values for pixel 1, next array of values for pixel 2, and so forth. We have added a new parameter "nDim" for this number of values per pixel.

## When to use

In image or raster compression, there are two different options:

- compress an image as much as possible but so it still looks ok
  (jpeg and relatives). The max coding error per pixel can be large.

- prioritize control over the max coding error per pixel (elevation,
  scientific data, medical image data, ...).

In the second case, data is often compressed using lossless methods, such as LZW, gzip, and the like. The compression ratios achieved are often low. On top the encoding is often slow and time consuming.

Lerc allows you to set the max coding error per pixel allowed, called `"MaxZError"`. You can specify any number from `0` (lossless) to a number so large that the decoded image may come out flat.

In a nutshell, if jpeg is good enough for your images, use jpeg. If not, if you would use png instead, or gzip, then you may want to try out Lerc.

## How to use

Lerc can be run anywhere without external dependencies. This project includes test samples of how to use LERC directly, currently for C++, C#, JavaScript, and Python. We have added a few small data samples under `testData/`. There is also a precompiled Windows dll and a Linux .so file under `bin/`.

### How to use without compiling LERC

Check out the Lerc decoders in `OtherLanguages/`. You may need to adjust the paths to input data and the dll or .so file. Other than that they should just work.

### How to compile LERC and the C++ test program

#### Windows

- Open `build/Windows/MS_VS201X/Lerc.sln` with Microsoft Visual Studio. We have upgraded to VS2017.
- Pick x64 (dflt) or win32.
- Build and run.

#### Linux

- Open `build/Linux/CodeBlocks/Lerc/Lerc_so.cbp` using the free Code::Blocks IDE for Linux.
- Build it. Should create `libLerc_so.so`.
- Open `build/Linux/CodeBlocks/Test/Test.cbp`.
- Build and run.

LERC can also be used as a compression mode for the GDAL image formats GeoTIFF (since GDAL 2.4) and MRF (since GDAL 2.1) via [GDAL](http://gdal.org).

## LERC Properties

- works on any common data type, not just 8 bit:
  char, byte, short, ushort, int, uint, float, double.

- works with any given MaxZError or max coding error per pixel.

- can work with a byte mask that specifies which pixels are valid
  and which ones are not.

- is very fast: encoding time is about 20-30 ms per MegaPixel per band, decoding time is about 5 ms per MegaPixel per band.

- compression is better than most other compression methods for
  larger bitdepth data (int types larger than 8 bit, float, double).

- for 8 bit data lossless compression, PNG can be better, but is
  much slower.

- in general for lossy compression with MaxZError > 0, the larger
  the error allowed, the stronger the compression.
  Compression factors larger than 100x have been reported.

- this Lerc package can read all (legacy) versions of Lerc, such as Lerc1, Lerc2 v1, v2, v3, and the current Lerc2 v4. It always writes the latest stable version.

The main principle of Lerc and history can be found in [doc/MORE.md](doc/MORE.md)

## Benchmarks

Some benchmarks are in
[doc/LercBenchmarks_Feb_2016.pdf](doc/LercBenchmarks_Feb_2016.pdf)

## Bugs?

The codecs Lerc2 and Lerc1 have been in use for years, bugs in those low level modules are very unlikely. All software updates are tested in Esri software for months before they are uploaded to this repo. 

## Licensing

Copyright 2015-2018 Esri

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and limitations under the License.

A copy of the license is available in the repository's [LICENSE](./LICENSE) file.

