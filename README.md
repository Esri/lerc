# LERC - Limited Error Raster Compression

## What is LERC?

LERC is an open-source image or raster format which supports rapid encoding and decoding for any pixel type (not just RGB or Byte). Users set the maximum compression error per pixel while encoding, so the precision of the original input image is preserved (within user defined error bounds).

This repository contains a C++ library for both encoding and decoding images. You can also do this directly from Python. And we have decoders for JavaScript and C#.

## The LERC C API

Function | Description
--- | ---
`uint lerc_computeCompressedSize(...)` | Computes the buffer size that needs to be allocated so the image can be Lerc compressed into that buffer. The size is accurate to the byte. This function is optional. It is faster than `lerc_encode(...)`. It can also be called to decide whether an image or image tile should be encoded by Lerc or another method.
`uint lerc_encode(...)` | Compresses a given image into a pre-allocated buffer. If that buffer is too small, the function fails with the corresponding error code. The function also returns the number of bytes written.
`uint lerc_getBlobInfo(...)` | Looks into a given Lerc byte blob and returns an array with all the header info. From this, the image to be decoded can be allocated and constructed. This function is optional. You don't need to call it if you already know the image properties such as tile size and data type.
`uint lerc_getDataRanges(...)` | Looks into a given Lerc byte blob and returns 2 double arrays with the minimum and maximum values per band and depth. This function is optional. It allows fast access to the data ranges without having to decode the pixels.
`uint lerc_decode(...)` | Uncompresses a given Lerc byte blob into a pre-allocated image. If the data found in the Lerc byte blob does not fit the specified image properties, the function fails with the corresponding error code.
`uint lerc_decodeToDouble(...)` | Uncompresses a given Lerc byte blob into a pre-allocated image of type double independent of the compressed data type. This function was added mainly to be called from other languages such as Python and C#.

To support the case that not all image pixels are valid, a mask image can be passed. It has one byte per pixel, 1 for valid, 0 for invalid.

See the sample program `src/LercTest/main.cpp` which demonstrates how the above functions are called and used. Also see the two header files in the `src/LercLib/include/` folder and the comments in there.

About multiple bands, or multiple values per pixel. This has changed with Lerc version 2.4. Before, you could either store each band into its own Lerc byte blob which allowed you to access / decode each band individually. Lerc also allowed to stack bands together into one single Lerc byte blob. This could be useful if the bands are always used together anyway. Now, since Lerc version 2.4, you can additionally store multiple values per pixel interleaved, meaning an array of values for pixel 1, next array of values for pixel 2, and so forth. We have added a new parameter "nDepth" for this number of values per pixel.

While the above can be used as an "interleave flag" to store multiple raster bands as a 3D array as either [nBands, nRows, nCols] for band interleaved or as [nRows, nCols, nDepth] for pixel interleaved, it also allows to do both at the same time and store a 4D array as [nBands, nRows, nCols, nDepth]. 

Note that the valid / invalid pixel byte mask is not 4D but limited to [nBands, nRows, nCols]. This mask is per pixel per band. For nDepth > 1 or an array of values per pixel, up to Lerc version 3.0, Lerc assumed all values in that array at that pixel are either valid or invalid. If the values in the innermost array per pixel can be partially valid and invalid, use a predefined noData value or NaN. 

To better support this special "mixed" case, we have added new Lerc API functions *_4D() in Lerc version 4.0, see [Lerc_c_api.h](https://github.com/Esri/lerc/blob/master/src/LercLib/include/Lerc_c_api.h). These functions allow to pass one noData value per band to the encode_4D() function and can receive it back in the decode_4D() function. This way such data can be compressed with maxZError > 0 or lossy, despite the presence of noData values in the data. Note that Lerc will convert noData values to 0 bytes in the valid / invalid byte mask whenever possible. This also allows now to pass raster data with noData values to the encoder without first creating the valid / invalid byte mask. NoData values can be passed both ways, as noData or as byte mask. Note that on decode Lerc only returns a noData value for the mixed case of valid and invalid values at the same pixel (which can only happen for nDepth > 1). The valid / invalid byte mask remains the preferred way to represent void or noData values. 

Remark about NaN. As Lerc supports both integer and floating point data types, and there is no NaN for integer types, Lerc filters out NaN values and replaces them. Preferred it pushes NaN's into the valid / invalid byte mask. For the mixed case, it replaces NaN by the passed noData value. If there is no noData value, encode will fail. Lerc decode won't return any NaN's. 

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

Lerc can be run anywhere without external dependencies. This project includes test samples of how to use LERC directly, currently for C++, Python, JavaScript, and C#. We have added a few small data samples under `testData/`. There is also a precompiled Windows dll and a Linux .so file under `bin/`.

### How to use without compiling LERC

Check out the Lerc decoders and encoders in `OtherLanguages/`. You may need to adjust the paths to input or output data and the dll or .so file. Other than that they should just work.

### Other download sites

- Lerc for Python (pylerc): [Conda](https://anaconda.org/Esri/pylerc) / [PyPI](https://pypi.org/project/pylerc/) (Note: The python package name is being renamed from `lerc` to `pylerc` in the next release- currently available under both names. However, the import name will remain the same (ie. `import lerc`).
- [Lerc for JavaScript / npm](https://www.npmjs.com/package/lerc)
- [Lerc conda-forge install](https://anaconda.org/conda-forge/lerc)

### How to compile LERC and the C++ test program

For building the Lerc library on any platform using CMake, use `CMakeLists.txt`. 
For the most common platforms you can find alternative project files under `build/`. 

#### Windows

- Open `build/Windows/MS_VS2022/Lerc.sln` with Microsoft Visual Studio. 
- Build and run.

#### Linux

- Open `build/Linux/CodeBlocks/Lerc/Lerc_so.cbp` using the free Code::Blocks IDE for Linux.
- Build it. Should create `libLerc_so.so`.
- Open `build/Linux/CodeBlocks/Test/Test.cbp`.
- Build and run.

#### MacOS

- Open `build/MacOS/Lerc64/Lerc64.xcodeproj` with Xcode.
- Build to create dynamic library.

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

- this Lerc package can read all (legacy) codec versions of Lerc, such as Lerc1, Lerc2 v1 to v5, and the current Lerc2 v6. It always writes the latest stable version.

The main principle of Lerc and history can be found in [doc/MORE.md](doc/MORE.md)

## Benchmarks

Some benchmarks are in
[doc/LercBenchmarks_Feb_2016.pdf](doc/LercBenchmarks_Feb_2016.pdf)

## Bugs?

The codecs Lerc2 and Lerc1 have been in use for years, bugs in those low level modules are very unlikely. All software updates are tested in Esri software for months before they are uploaded to this repo. 

## Licensing

Copyright 2015-2022 Esri

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and limitations under the License.

A copy of the license is available in the repository's [LICENSE](./LICENSE) file.

