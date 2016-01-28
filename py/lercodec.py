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
##
##
##-----------------------------------------------------------------------------

import struct
import array

'''Lerc codec'''

class header(object):
    'Lerc codec'
    def __init__(self, blob):
        try:
            self.valid = blob[0:10] == "CntZImage "
            assert self.valid

            fileVersion, imageType, self.height, self.width, self.u = \
                struct.unpack_from("<IIIId", blob, 10)
            self.u *=2  # ZMaxError is half the quanta
            assert fileVersion == 11 and imageType == 8

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

    def __str__(self):
        if not self.valid:
            return "Invalid Lerc Header"
        s = "Lerc:\n\tHeader: size {}x{}, maxError {}\n".format(
                self.width, self.height, self.u/2)
        s += "\tValues: block count {}x{}, bytes {}, maxvalue {}".format(
                self.v_nbX, self.v_nbY, self.v_nB, self.v_maxVal)
        if "m_nbX" in dir(self):
            s += "\n\tMask: block count {}x{}, bytes {}, maxvalue {}".format(
                self.m_nbX, self.m_nbY, self.m_nB, self.m_maxVal)
        return s

#bitmask in row major, with most significant bit first
class bitmask(object):
    def __init__(self, width, height, val = 255, data = None):
        self.w = width
        self.h = height
        self.sz = (width * height + 7) / 8
        self.data = array.array('B', (val,) * self.sz) if data is None \
            else data
        assert self.sz == len(self.data)

    def at(self, x, y):
        'query mask at x and y'
        assert x < self.w and y < self.h
        l = y * self.w + x
        return 0 != (self.data[l / 8] & (128 >> (l % 8)))

    def set(x, y, val = 1):
        'set bit, or clear bit if val is zero'
        assert x < self.w and y < self.h
        l = y * self.w + x
        bpos = 128 >> (l % 8)
        l /= 8
        self.data[l] = (self.data[l] | bpos) if val != 0 \
            else (self.data[l] & ~bpos)

    def size(self):
        return self.sz

# size is the number of bytes to read
def decode_RLE(blob, offset, size):
    'Unpack the RLE encoded chunk of size starting at offset in blob'
    result = array.array('B')

    while size > 2:
        count = struct.unpack_from("<h", blob, offset)[0]

        if count < 0: # Negative count means repeated bytes
            result.extend(struct.unpack_from("B", blob, offset + 2)* -count)
            count = 1
        else: # Otherwise, copy count bytes to output
            result.extend(struct.unpack_from("B"* count, blob, offset + 2))

        offset += 2 + count
        size -= 2 + count

    if struct.unpack_from("<h", blob, offset)[0] != -32768:
        result = None  # End marker check failed

    return result

def decode_mask(blob, h = None):
    'Decode mask from the lerc blob'
    if h is None:
        h = header(blob)
    if not h.valid:
        return None

    if 0 == h.m_nB:  # Degenerate case, all valid or all invalid
        return bitmask(h.width, h.height, val = int(h.m_maxVal*255))

    return bitmask(h.width, h.height, data = decode_RLE(blob, 50, h.m_nB))

# BitStuffer - read quantized values
def read_qval(blob, offset):
    'Reads a block of packed integers, returns the updated offset and a list of integers'
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
    # How many full integers are there
    nint = (count * nbits + 24) / 32
    values32 = struct.unpack_from("<" + "I"*nint, blob, offset)
    offset += nint * 4

    # If not all bytes of the last integer is used, they are not stored
    # But the used bytes are still from the most significan byte end
    extra_bytes = ((count * nbits + 7) / 8) % 4
    if 0 != extra_bytes:
        values8 = struct.unpack_from("B" * extra_bytes, blob, offset)
        offset += extra_bytes
        v8 = 0
        for i in range(extra_bytes):
            v8 |= values8[i] << (8 * (i + 4 - extra_bytes))
        values32 = values32 + (v8,)

    # Decode from the integer array, values are always positive
    values = array.array("I", (0,)* count)
    src = 0
    bits_left = 32
    mask = (1 << nbits) -1
    for dst in range(count):
        if bits_left >= nbits:
            values[dst] = mask & (values32[src] >> (bits_left - nbits))
            bits_left -= nbits
        else: # No enough bits left in this input integer
            pbits = nbits - bits_left
            values[dst] = (values32[src] & ((1 << bits_left) -1)) << pbits
            src += 1
            bits_left = 32 - pbits
            values[dst] += values32[src] >> bits_left

    return values, offset

# Single block read/expand
def read_block(data, x0, y0, x1, y1, mask, Q, max_val, blob, offset):
    'Read a lerc block into data, at bx,by of size tw,th, from blob at offset'
    flags, = struct.unpack_from('B', blob, offset)
    offset += 1
    mf = flags >> 6  # Format of minimum
    flags &= 63
    assert flags < 4

    if flags == 0:
        # Stored as such, based on mask
        for y in range(y0, y1):
            for x in range(x0, x1):
                if mask.at(x, y) != 0:
                    data[y * mask.w + x], = struct.unpack_from(
                        "<f", blob, offset)
                    offset += 4
        return offset

    mval = 0.0
    if 0 != (flags & 1): # A min value is present
        mval, = struct.unpack_from(("<f", "<h", "b")[mf], blob, offset)
        offset += (4, 2, 1)[mf]

    if 0 != (flags & 2): # constant encoding if bit zero is clear
        for y in range(y0, y1):
            for x in range(x0, x1):
                data[y * mask.w + x] = mval
        return offset

    # Unpack as integers
    ival, offset = read_qval(blob, offset)
    src = 0
    for y in range(y0, y1):
        for x in range(x0, x1):
            if mask.at(x, y) != 0:
                v = mval + Q * ival[src]
                data[y * mask.w + x] = v if v < max_val else max_val
                src += 1

    return offset

# Decode data from blob starting at a given offset
# assuming header, mask and offset are correct
def read_tiles(blob, h, mask, offset = 0):
    result = array.array('f', (0.0, )* h.width * h.height)
    # Figure out the endcoding block size
    bh = h.height / h.v_nbY
    bw = h.width / h.v_nbX
    for y in range(0, h.height, bh):
        yend = y + min(bh, h.height - y)
        for x in range(0, h.width, bw):
            xend = x + min(bw, h.width -x)
            offset = read_block(result,
                x, y, xend, yend,
                mask, h.u, h.v_maxVal,
                blob, offset)
    return result


# Reads the header and the mask if needed
# returns only the data
def decode_data(blob, h = None, mask = None):
    'Decode the data from the lerc blob'
    if h is None:
        h = header(blob)
    if False is h.valid:
        return None
    if mask is None:
        mask = decode_mask(blob, h)
    return read_tiles(blob, h, mask, 66 + h.m_nB)

def decode(blob):
    'The decoder, returns header and the mask objects and a data array'
    h = header(blob)
    if not h.valid:
        return (None,) *3
    mask = decode_mask(blob, h)
    return h, mask, decode_data(blob, h, mask)

def main():
    # world.lerc contains the world elevation in WebMercator 257x257
    # with a 1 value NoData border
    blob = open("world.lerc", "rb").read()
    info, mask, data = decode(blob)

    # Mask can be interogated with mask.at(x,y),
    # data is an array of floats of h.width x h.height size

    assert info.valid
    assert mask.at(0,0) == 0 and mask.at(1,1) == 1
    assert data[74 * info.width + 74] == 111.0

    #
    # dump the data to output as csv
    #
    # NOTE: the line order is top to bottom in MRF tiles, so tiles
    # can be assembled without having to invert the array
    #
    # Most if not all of Esri usage is bottom to top
    #
    #  The sample is a single image, so the lines are bottom to top
    #

##    for row in range(info.height -1, -1, -1):
##        print ",".join(`data[row * info.width + column]`
##            for column in range(info.width))
##

    print info


if __name__ == '__main__':
    main()
