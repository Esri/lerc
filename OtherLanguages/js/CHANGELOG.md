# Change Log

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](http://keepachangelog.com/)
and this project adheres to [Semantic Versioning](http://semver.org/).

## [Unreleased][HEAD]

## [2.0.0] - 2018-11-06

The decoder is in sync with ArcMap 10.7 and ArcGIS Pro 2.3. LERC encoded binary blobs from any previous version of ArcMap or ArcGIS Pro can also be read / decoded.

### Added
* Extend from one value per pixel to nDim values per pixel.
* Add new lossy compression option for integer types (aiming at noisy 16 bit satellite images). Cuts off noisy bit planes.
* Add more rigorous checking against buffer overrun while decoding. (This concept taken from the MRF / gdal repo.)
* Enable encode / write former versions 2.3 and 2.2. This is to allow MRF / gdal to make their Lerc2 calls into this Lerc lib.

### Changed
* Upgrade Lerc codec to new version Lerc 2.4.

## 1.0.1 - 2017-02-18

### Fixed

* resolved a Huffman code table parsing issue [#31](https://github.com/Esri/lerc/pull/31)

## 1.0 - 2016-11-30

This LERC API JavaScript decoder is in sync with ArcMap 10.5 and ArcGIS Pro 1.4. LERC encoded binary blobs from any previous version of ArcMap or ArcGIS Pro can be read / decoded as well.

### What will trigger a major version change

- A change to this LERC API that is not backwards compatible and requires users to update / change their code in order to use an upgraded .dll or .so file.
- A change to the LERC bitstream that is not backwards compatible and requires users to upgrade their LERC encoder and / or decoder.

[2.0.0]: https://github.com/Esri/lerc/compare/v1.0.1...v2.0 "v2.0"
[HEAD]: https://github.com/Esri/lerc/compare/v2.0...HEAD "Unreleased Changes"
