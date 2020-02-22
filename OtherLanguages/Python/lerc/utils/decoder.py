#
# Name:        lerc
#
# Purpose:     Lerc1 decoder in native python
#
# Author:      Lucian Plesea
#
# Created:     26/01/2016
#
#
# The main function is decode(blob), which takes a binary sequence
# and returns a tuple (header, mask, data).  The header and mask are objects
# defined in this module, data is an array of floats from the lerc blob
#
# Only handles lerc1 with binary data mask at this time
#

##-----------------------------------------------------------------------------
##    Copyright 2016 Esri
##    Licensed under the Apache License, Version 2.0 (the "License");
##    you may not use this file except in compliance with the License.
##    You may obtain a copy of the License at
##    http://www.apache.org/licenses/LICENSE-2.0
##    Unless required by applicable law or agreed to in writing, software
##    distributed under the License is distributed on an "AS IS" BASIS,
##    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
##    See the License for the specific language governing permissions and
##    limitations under the License.
##    A copy of the license and additional notices are located with the
##    source distribution at:
##    http://github.com/Esri/lerc/
##-----------------------------------------------------------------------------

'''Lerc codec'''

import struct
import array

def dloop(xr, yr):
    ''' A double loop, x inner, y outer'''
    for y in yr:
        for x in xr:
            yield x, y

# size is the number of bytes to read
def decode_RLE(blob, offset, size):
    'Unpack the RLE encoded chunk of size starting at offset in blob'
    result = array.array('B')
    while size > 2:
        count = struct.unpack_from("<h", blob, offset)[0]
        offset += 2
        # Negative count means repeated bytes
        if count < 0:
            result.extend(struct.unpack_from("B", blob, offset)* -count)
            count = 1
        else:
            result.extend(struct.unpack_from("B"* count, blob, offset))
        offset += count
        size -= 2 + count
    if struct.unpack_from("<h", blob, offset)[0] != -32768:
        return None  # End marker check failed
    return result

def read_qval(blob, offset):
    'Reads a block of packed integers, updates offset'
    # First byte holds the number of bits per value and the varint byte count
    # for the number of values
    nbits, = struct.unpack_from("B", blob, offset)
    offset += 1
    bc = nbits >> 6
    nbits &= 63
    if nbits == 0:  # There are no values, return the offset only
        return None, offset

    count, = struct.unpack_from(("<I", "<H", "B")[bc], blob, offset)
    offset += (4, 2, 1)[bc]

    # Values are packed in low endian int values, but starting with MSbit
    nint = (count * nbits + 7) / 32
    values32 = struct.unpack_from("<" + "I"*nint, blob, offset)
    offset += nint * 4

    # If not all bytes of the last integer is used, they are not stored
    # But the used bytes are still from the most significan bit end
    extra_bytes = ((count * nbits + 7) / 8) % 4
    if 0 != extra_bytes:
        values8 = struct.unpack_from("B" * extra_bytes, blob, offset)
        offset += extra_bytes
        v8 = 0
        for i in range(extra_bytes):
            v8 |= values8[i] << (8 * (i + 4 - extra_bytes))
        values32 = values32 + (v8,)

    # Decode the integer array, values are always positive
    values = array.array("I", (0,)* count)
    src = 0
    bits_left = 32
    mask = (1 << nbits) -1
    for dst in range(count):
        if bits_left >= nbits:
            values[dst] = mask & (values32[src] >> (bits_left - nbits))
            bits_left -= nbits
        else: # Not enough bits left in this input integer
            pbits = nbits - bits_left
            values[dst] = (values32[src] & ((1 << bits_left) -1)) << pbits
            src += 1
            bits_left = 32 - pbits
            values[dst] += values32[src] >> bits_left

    return values, offset

# Single block read/expand, returns the end offset
def read_block(data, rx, ry, mask, Q, max_val, blob, offset):
    'Read a lerc block into data, at bx,by of size tw,th, from blob at offset'
    flags, = struct.unpack_from('B', blob, offset)
    offset += 1
    mf = flags >> 6  # Format of minimum
    flags &= 63
    assert flags < 4

    if flags == 0:
        # Stored as such, based on mask
        for x, y in dloop(rx, ry):
            if mask.at(x, y) != 0:
                data[y * mask.w + x], = struct.unpack_from("<f", blob, offset)
                offset += 4
        return offset

    if 0 != (flags & 1):
        # A min value is present
        mval, = struct.unpack_from(("<f", "<h", "b")[mf], blob, offset)
        offset += (4, 2, 1)[mf]
    else:
        mval = 0.0

    # constant encoding, all mval, ignore the mask
    if 0 != (flags & 2):
        for x, y in dloop(rx, ry):
            data[y * mask.w + x] = mval
        return offset

    # Unpack from integers, based on mask
    ival, offset = read_qval(blob, offset)
    src = 0
    for x, y in dloop(rx, ry):
        if mask.at(x, y) != 0:
            data[y * mask.w + x] = min(mval + Q * ival[src], max_val)
            src += 1
    return offset

#bitmask in row major, most significant bit first
class bitmask(object):
    def __init__(self, w, h, val = 255, data = None):
        sz = (w * h + 7) / 8
        self.w, self.h = w, h
        self.data = array.array('B', (val,) * sz) if data is None else data

    def at(self, x, y):
        'query mask at x and y'
        l = y * self.w + x
        return 0 != (self.data[l / 8] & (128 >> (l % 8)))

class lerc(object):
    'Lerc codec'
    def __init__(self, blob): # Only reads the header
        try:
            self.valid = blob[0:10] == "CntZImage "
            assert self.valid

            fileVersion, imageType, self.h, self.w, self.u = (
                struct.unpack_from("<IIIId", blob, 10))

            # ZMaxError is half the quanta
            self.u *= 2
            assert fileVersion == 11 and imageType == 8

            # Number of blocks ignores possible partial blocks
            nbY, nbX, nB, val = struct.unpack_from("<IIIf", blob, 34)
            # If there is an extra array in the blob, the first one is the mask
            if len(blob) >= 66 + nB:
                # Verify it is a mask, block count is zero and value of 1 or 0
                assert (0 == nbX | nbY) and (1.0 == val or 0.0 == val)
                self.m_nbY = nbY
                self.m_nbX = nbX
                self.m_nB = nB
                self.m_maxVal = val
                # Read the data header
                nbY, nbX, nB, val = struct.unpack_from("<IIIf", blob, 50 + nB)

            self.v_nbY = nbY
            self.v_nbX = nbX
            self.v_nB = nB
            self.v_maxVal = val

        except Exception as e: # Return the invalid flag
#            print "Error exit {}".format(e)
            self.valid = False
            return

    def decode_mask(self, blob):
        '''Decode mask from the lerc blob, RLE compressed'''
        # If there are no mask bytes then it is all ones or zeros
        if 0 == self.m_nB:
            return bitmask(self.w, self.h, val = int(self.m_maxVal * 255))
        else:
            return bitmask(self.w, self.h, data = decode_RLE(blob, 50, self.m_nB))

    # Decode data from blob starting at a given offset
    # assuming header, mask and offset are correct
    def read_tiles(self, blob):
        # the encoding block size
        bh = self.h / self.v_nbY
        bw = self.w / self.v_nbX
        offset = 66 + self.m_nB
        result = array.array('f', (0.0, ) * self.w * self.h)
        for x, y in dloop(range(0, self.w, bw), range(0, self.h, bh)):
            offset = read_block(result,
                range(x, x + min(bw, self.w - x)),
                range(y, y + min(bh, self.h - y)),
                self.mask, self.u, self.v_maxVal,
                blob, offset)
        return result

    def decode(self, blob):
        'The decoder, returns the data array'
        if not self.valid:
            return (None,) *3
        if "mask" not in dir(self):
            self.mask = self.decode_mask(blob)
        return self.read_tiles(blob)

    def __str__(self):
        if not self.valid:
            return "Invalid Lerc Header"
        s = "Lerc:\n\tHeader: size {}x{}, maxError {}\n".format(
                self.w, self.h, self.u/2)
        s += "\tValues: block count {}x{}, bytes {}, maxvalue {}".format(
                self.v_nbX, self.v_nbY, self.v_nB, self.v_maxVal)
        if "m_nbX" in dir(self):
            s += "\n\tMask: block count {}x{}, bytes {}, maxvalue {}".format(
                self.m_nbX, self.m_nbY, self.m_nB, self.m_maxVal)
        return s
