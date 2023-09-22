## `pylerc` is the LERC Python package

LERC is an open-source raster format which supports rapid encoding and decoding for any pixel type, with user-set maximum compression error per pixel.

# What's new in Lerc 4.0?

## Option 1, uses numpy masked array

An encoded image tile (2D, 3D, or 4D), of data type byte to double, can have
any value set to invalid. There are new and hopefully easy to use numpy
functions to decode from or encode to Lerc. They make use of the numpy
masked array.

(result, npmaArr, nDepth, npmaNoData) = decode_ma(lercBlob)

- lercBlob is the compressed Lerc blob as a string buffer or byte array as
  read in from disk or passed in memory.
- result is 0 for success or an error code for failure.
- npmaArr is the masked numpy array with the data and mask of the same shape.
- nDepth == nValuesPerPixel. E.g., 3 for RGB, or 2 for complex numbers. 
- npmaNoData is a 1D masked array of size nBands. It can hold one noData
  value per band. The caller can usually ignore it as npmaArr has all mask
  info. It may be useful if the data needs to be Lerc encoded again.

(result, nBytesWritten, lercBlob) = encode_ma(npmaArr, nDepth, maxZErr,
                                              nBytesHint, npmaNoData = None)

- npmaArr is the image tile (2D, 3D, or 4D) to be encoded, as a numpy masked array.
- nDepth == nValuesPerPixel. E.g., 3 for RGB, or 2 for complex numbers.
- maxZErr is the max encoding error allowed per value. 0 means lossless.
- nBytesHint can be
  - 0 - compute num bytes needed for output buffer, but do not encode it (faster than encode)
  - 1 - do both, compute exact buffer size needed and encode (slower than encode alone)
  - N - create buffer of size N and encode, if buffer too small encode will fail.

- npmaNoData is a 1D masked array of size nBands. It can hold one noData
  value per band. It can be used as an alternative to masks. It must be
  used for the so called mixed case of valid and invalid values at the same
  pixel, only possible for nDepth > 1. In most cases None can be passed.
  Note Lerc does not take NaN as a noData value here. It is enough to set the
  data values to NaN and not specify a noData value.

## Option 2, uses regular numpy arrays for data and mask

As an alternative to the numpy masked array above, there is also the option
to have data and masked as separate numpy arrays.

(result, npArr, npValidMask, npmaNoData) = decode_4D(lercBlob)

Here, npArr can be of the same shapes as npmaArr above, but it is a regular
numpy array, not a masked array. The mask is passed separately as a regular
numpy array of type bool. Note that in contrast to the masked array above,
True means now valid and False means invalid. The npValidMask can have the
following shapes:
- None, all pixels are valid or are marked invalid using noData value or NaN.
- 2D or (nRows, nCols), same mask for all bands.
- 3D or (nBands, nRows, nCols), one mask per band.

The _4D() functions may work well if all pixels are valid, or nDepth == 1,
and the shape of the mask here matches the shape of the data anyway.
In such cases the use of a numpy masked array might not be needed or
considered an overkill.

Similar for encode:

(result, nBytesWritten, lercBlob) = encode_4D(npArr, nDepth, npValidMask, 
                                              maxZErr, nBytesHint, npmaNoData = None)

## General remarks

Note that for all encode functions, you can set values to invalid using a
mask, or using a noData value, or using NaN (for data types float or double).
Or any combination which is then merged using AND for valid (same as OR for invalid)
by the Lerc API.

The decode functions, however, return this info as a mask, wherever possible.
Only for nDepth > 1 and the mixed case of valid and invalid values at the same
pixel, a noData value is used internally. NaN is never returned by decode.

The existing Lerc 3.0 encode and decode functions can still be used.
Only for nDepth > 1 the mixed case cannot be encoded. If the decoder
should encounter a Lerc blob with such a mixed case, it will fail with
the error code LercNS::ErrCode::HasNoData == 5. 

