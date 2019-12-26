# Change Log

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [Unreleased][unreleased]

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

## Fixed

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
