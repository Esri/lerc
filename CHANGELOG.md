# Change Log

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [Unreleased][unreleased]


## [3.0](https://github.com/Esri/lerc/releases/tag/v3.0) - 2021-07-30

### Milestones reached
- This LERC API and all language decoders (C++, C, C#, Python, JavaScript) and encoders (C++, C, Python) are now in sync with these ESRI ArcGIS versions: ESRI ArcGIS Pro 2.8, ESRI ArcMap 10.8.1.
LERC encoded binary blobs of any previous version of ArcGIS Pro or ArcMap can be read / decoded.

### Fixed

* Moved include/ folder to src/LercLib/ and removed duplicate header files. Some updates in Lerc_c_api.h for dll export / import switch, and in CMakeLists.txt. 

### Added

* If multiple bands are concatenated into one binary Lerc blob, and not all pixels are valid, Lerc now allows different valid / invalid byte masks per band. Before, there was just one such mask for all bands. To support this, one new parameter has been added to the API function calls called "nMasks" (int). It can be set to 0 (no mask), 1 (one mask for all bands), or nBands (allowing one mask per band). Allocate the mask byte array accordingly to (nMasks * nRows * nCols). It is ok to always use nBands masks or more masks than needed, for both encode and decode. 

* Support NaN values in floating point images. Lerc checks for NaN values at valid pixels. If found, it translates them into invalid mask pixels. For the special case of array per pixel (nDim > 1) and a mix of NaN's and valid floating point values inside such an array, the NaN's get converted to -FLT_MAX (or -DBL_MAX). 


## [2.2.1](https://github.com/Esri/lerc/releases/tag/v2.2.1) - 2020-12-28

### Added

* Simplify build and install Lerc as Conda package for Python. 


## [2.2](https://github.com/Esri/lerc/releases/tag/v2.2) - 2020-07-06

### Milestones reached
- This LERC API and all language decoders (C++, C, C#, Python, JavaScript) and encoders (C++, C, Python) are now in sync with these ESRI ArcGIS versions to be released soon: ESRI ArcGIS Pro 2.6, ESRI ArcMap 10.8.1. 
LERC encoded binary blobs of any previous version of ArcGIS Pro or ArcMap can be read / decoded.

### Fixed

* Fixed a bug in Lerc2 decoder. If a compressed Lerc2 blob has more than 1 band, and the blob has a bit mask (not all pixels are valid), the decoder fails. [#129](https://github.com/Esri/lerc/pull/129)

* Fixed a bug in Lerc1 decoder. If a compressed Lerc1 blob has more than 1 band, and the last band has less than 16 bytes of data, then this last band could be skipped by the Lerc1 decoder. [#121](https://github.com/Esri/lerc/pull/121)


## [2.1](https://github.com/Esri/lerc/releases/tag/v2.1) - 2019-12-26

### Milestones reached
- This LERC API and all language decoders (C++, C, C#, Python, JavaScript) and encoders (C++, C, Python) are now in sync with these ESRI ArcGIS versions to be released soon: ESRI ArcGIS Pro 2.5, ESRI ArcMap 10.8. 
And also with these versions already released: ESRI ArcGIS Pro 2.4, ESRI ArcMap 10.7.1. LERC encoded binary blobs of any previous version of ArcGIS Pro or ArcMap can be read / decoded.

### Added

* For multiple values per pixel or nDim > 1, allow for relative encoding to exploit the possible correlation between neighboring bands. This promises better compression for hyperspectral images and multi-dimensional images. 
* For floating point images, try to raise MaxZError if possible without further loss. This can yield better compression if the input data has already been truncated earlier (to e.g., 0.01). Or if the input data has been truncated to different values in different areas. 
* Added sample python function to Lerc encode an image from python. 

### Changed

* Upgrade Lerc codec to new version Lerc 2.5 (or Lerc2 v5). 
* Updated Readme.
* Updated doc.
* Updated binary .dll and .so files. 
* Note that the former license restrictions on field of use have been removed. 


## [2.0](https://github.com/Esri/lerc/releases/tag/v2.0) - 2018-11-05

### Milestones reached
- This LERC API and all language decoders (C++, C, C#, Python, JavaScript) are now in sync with these ESRI ArcGIS versions to be released soon: ESRI ArcMap 10.7, ESRI ArcGIS Pro 2.3. LERC encoded binary blobs of any previous version of ArcMap or ArcGIS Pro can be read / decoded.

### Fixed

* Resolved a Huffman code table parsing issue in JavaScript decoder [#31](https://github.com/Esri/lerc/pull/31)

### Added

* Extend from one value per pixel to nDim values per pixel.
* Add new lossy compression option for integer types (aiming at noisy 16 bit satellite images). Cuts off noisy bit planes.
* Add more rigorous checking against buffer overrun while decoding. (This concept taken from the MRF / gdal repo.)
* Enable encode / write former versions 2.3 and 2.2. This is to allow MRF / gdal to make their Lerc2 calls into this Lerc lib. 

### Changed

* Upgrade Lerc codec to new version Lerc 2.4.
* Updated Readme.
* Simplified the builds.
* Simplified the folder structure.
* Upgraded to C++ 11, MS VS 2017, 64 bit.
* Updated python and C# decoders.
* Updated JS decoder. 
* Updated doc.
* Updated binary .dll and .so files

## [1.0](https://github.com/Esri/lerc/releases/tag/v1.0) - 2016-11-30

### Milestones reached
- This LERC API and all language decoders (C++, C, C#, Python, JavaScript) are now in sync with these ESRI ArcGIS versions to be released soon: ESRI ArcMap 10.5, ESRI ArcGIS Pro 1.4. LERC encoded binary blobs of any previous version of ArcMap or ArcGIS Pro can be read / decoded.

### What will trigger a major version change
- A change to this LERC API that is not backwards compatible and requires users to update / change their code in order to use an upgraded .dll or .so file.
- A change to the LERC bitstream that is not backwards compatible and requires users to upgrade their LERC encoder and / or decoder.

[unreleased]: https://github.com/Esri/lerc/compare/v2.1...HEAD
