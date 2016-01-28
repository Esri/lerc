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
  larger bitdepth data (int types larger than 8 bit, float, double).  

- for 8 bit data lossless compression, PNG can be better, but is
  much slower. 

- in general for lossy compression with MaxZError > 0, the larger
  the error allowed, the stronger the compression. 
  Compression factors larger than 100x have been reported.

- this Lerc package can read all (legacy) versions of Lerc, such as
  Lerc1, Lerc2 v1, and the current Lerc2 v2. It always writes the
  latest stable version. 


## How to use

For illustration, we have put together a simple test sample. 

- build the Lerc.dll in src/Lerc. Copy the resulting Lerc.lib into the lib/ folder. 
- build the test app in src/LercTest. 
- copy the Lerc.dll next to LercTest.exe.
- run it. 


## The LERC API

The LERC API has only 4 functions:

- bool ComputeBufferSize(...):
Computes the buffer size that needs to be allocated so the image can be Lerc compressed into that buffer. The size is accurate to the byte. This function is optional. It is faster than Encode(...). It can also be called to decide whether an image or image tile should be encoded by Lerc or another method. 

- bool Encode(...):
Compresses a given image into a pre-allocated buffer. If that buffer is too small, the function fails and returns false. The function returns the number of bytes written. 

- bool GetLercInfo(...):
Looks into a given Lerc byte blob and returns a struct with all the header info. From this, the image to be decoded can be allocated and constructed. This function is optional. You don't need to call it if you already know the image properties such as tile size and data type. 

- bool Decode(...):
Uncompresses a given Lerc byte blob into a pre-allocated image. If the data found in the Lerc byte blob does not fit the specified image properties, the function fails and returns false. 

To better support the case that not all image pixels are valid, the BitMask class is provided. It permits easy and efficient checking and setting pixels to valid / invalid. 

See the sample program src/LercTest/main.cpp how the above functions and classes are called and used. Also see the two header files in the include/ folder and the comments in there. 

One more comment about multiple bands. You can either store each band into its own Lerc byte blob which allows you to access / decode each band individually. Lerc also allows to stack bands together into one single Lerc byte blob. This can be useful if the bands are always used together anyway. 


## The Main Principle of Lerc

This section demonstrates how the same block of 4x4 pixels with floating point values gets Lerc encoded using two different values for MaxZError, the user specified coding error tolerance. The following is taken from the Lerc patent. The Lerc patent is available in the doc folder. 

![Lerc encoding sample](doc/LercEncodingSample.png)

#### Example of LERC Encoding For One Block Using MaxZError = 0.01 m

In some embodiments, LERC encoding can be performed using four simplified basic steps including, but not limited to, (1) calculating the basic statistics for a data block; (2) determining how to encode the block using a user defined MaxZError value, (3) determining the number of bits needed and bit stuffing the non-negative integers; and (4) writing the block header.  FIG. 9A depicts sample block data 910 for a 4 x 4 block of pixels, according to an embodiment of the invention.   It should be noted that blocks are typically 8 x 8 pixels or larger, but are shown here as 4 x 4 pixels for simplicity of description.  At step 1, the LERC method includes determining the basic statistics for the data block of FIG. 9A. The minimum value is 1222.2943 and the maximum value is 1280.8725. The number of valid pixels (i.e., non-void pixels) is 12 of 16.  The number of invalid pixels (i.e., void pixels) is 4 of 16. At step 2, the LERC method includes determining whether the pixel block should be encoded uncompressed or with a user-input MaxZError of 0.01 m. By using equation (1) above: 

(Max-Min) / (2 x MaxZError) => (1280.8725-1222.2943) / (2 x 0.01) = 2,928.91. 

Since this value is less than 2^30, we can quantize the pixel values in data block 910 and expect an acceptable compression ratio. The block is quantized using equation (2) above, where each new pixel value is calculated by: 

n(i) = (unsigned int)((x(i) - Min) / (2 x MaxZError) + 0.5), 

resulting in the new pixel block 920 shown in FIG. 9B. At step (3), the method further includes determining the number of bits needed and bit stuffing these non-negative integers. The number of bits needed can be determined by equation (3): 
 
NumBits = ceil(log2(2929)) = 12

To represent the number of bits needed another way, 2^11 < 2929 < 2^12. In this case, 12 bits per number are needed to encode all numbers of block 910 lossless. There are 12 valid numbers, resulting in 12 x 12 = 144 bits total.  As 144 / 8 = 18, 18 bytes are needed to encode the entire data block.  At step (4), the method includes writing the block header as shown in FIG. 9C. It follows that 7 bytes are needed for the block header. The total number of bytes needed for block 920 can be calculated as 18 + 7 = 25 bytes.  In this particular case, the header takes too much space with respect to the raw pixel data, which exemplifies the need to work with block sizes of 8 x 8 pixels or larger.  


#### Example of LERC Encoding For One Block Using MaxZError = 1.0 m

Using the same 4 x 4 pixel block shown in FIG. 9A, the LERC method is performed again using a larger MaxZError (i.e., error threshold) of 1.0 m. Beginning with step (1), the minimum value is 1222.2943 and the maximum value is 1280.8725. The number of valid pixels (i.e., non-void pixels) is 12 of 16. The number of invalid pixels (i.e., void pixels) is 4 of 16. In step (2), the LERC method proceeds with determining whether the pixel block should be encoded uncompressed or with a user-input MaxZError of 1.0 m. As By using equation (1) above:

(Max-Min) / (2 x MaxZError) => (1280.8725-1222.2943) / (2 x 1.0) = 29.2891.

Since this value is less than 2^30, we can quantize the pixel values in data block 910 and expect an acceptable compression ratio.  The block is quantized using equation (2) above, where each new pixel value is calculated by: 

n(i) = (unsigned int)((x(i) - Min) / (2 x MaxZError) + 0.5), 

resulting in the new pixel block 940 shown in FIG. 9D. At step (3), the method further includes determining the number of bits needed and bit stuffing these non-negative integers. The number of bits needed can be determine by equation (3): 

NumBits = ceil(log2(29)) = 5

To represent the number of bits needed another way, 2^4 < 29 < 2^5. In this case, 5 bits per number are needed to encode all numbers of block 940 lossless. There are 12 valid numbers, resulting in 5 x 12 = 60 bits total. As 60 / 8 = 7.5, 8 bytes are needed to encode the entire data block. At step (4), the method includes writing the block header as shown in FIG. 9E. It follows that 7 bytes are needed for the block header. The total number of bytes needed for block 920 can be calculated as 8 + 7 = 15 bytes. 


## Demos

We are not going to repeat here the general benefits of good compression. 
However, Lerc is special in a number of ways. First, regarding speed, not only
decode but also encode is very fast. This allows on-the-fly encode, transmit,
decode. For instance, data residing on a server stored at high accuracy
(potentially also using Lerc, with a very small MaxZError = 0.0001) can 
be requested by a client at a much lower accuracy sufficient for display
(let's say MaxZError = 0.1). Second, as Lerc supports all common data types,
it can be used for numerical or scientific data, not just regular images. 
Third, Lerc supports sparse images or raster with many pixels invalid
or empty. Last not least Lerc is a standalone codec with no dependencies on 
other software packages or libraries. It is relatively easy to port from C++ 
to other languages such as JavaScript or Python, esp if only the decoder is 
needed. A JavaScript decoder can enable web browsers to directly display 
compressed Lerc tiles. 

Here are some examples of fast display using Lerc:

http://developers.arcgis.com/javascript/samples/layers_raster/

http://developers.arcgis.com/javascript/samples/layers_imageservicevector/



## History

Lerc started in 2010 as a fast caching method for single band 32 bit float rasters. 
At the time nobody could foresee that it would turn into a compression format for 
all common data types. Later we also tried it successfully on 16 bit integer satellite data. 
This first version, Lerc1, was mainly used for single band 32 bit float elevation services. 
Lerc1 uses float internally, every other input data type gets converted to float first. 
Therefore the use of old Lerc1 is unsafe for data types that may not fit into 32 bit 
float, such as 32 bit integer or double. 

Mainly for this reason Lerc was re-implemented from scratch in 2014. It is now fully
templated C++, so it works for any input data type and any MaxZError down to 0 or 
lossless. We call it Lerc2. It has many other improvements over the previous version,
such as more info in the header. It is much faster, 2-4x, for both encode and decode. 
Lerc2 has been in use since then inside the Esri core software in multiple places for 
caching, transmission etc. 

The latest addition was an improvement on 8 bit data compression. For 8 bit data,
conventional compression methods yield usually better compression than Lerc, but are
much slower. We added textbook Huffman coding for 8 bit data types, resulting in 10-30% 
better compression while keeping the high speed. 


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

