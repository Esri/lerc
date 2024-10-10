/*
Copyright 2021 - 2022 Esri

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

Analytical Raster Compression.
Original coding 2021 Yuriy Yakimenko
*/

#include "fpl_EsriHuffman.h"
#include "Huffman.h"
#include <assert.h>
#include <cstdlib>
#include <stdint.h>
#include <limits>
#include <algorithm>

USING_NAMESPACE_LERC

void lerc_assert(bool v);

bool decodePackBits (const unsigned char *ptr, const size_t size, size_t expected, unsigned char **output)
{
    unsigned char *out = NULL;

    if (*output == NULL)
    {
        out = (unsigned char *)malloc (expected); // max possible

        *output = out;
    }
    else
    {
        out = *output;
    }

    size_t curr = 0;

    for (size_t i = 0; i < size; )
    {
        // read header byte.

        int b = ptr[i];

        if (b <= 127)
        {
            while (b >= 0) { i++; out[curr++] = ptr[i]; b--; }
            i++;
            continue;
        }
        else
        {
            i++;
            while (b >= 127) { out[curr++] = ptr[i]; b--; }
            i++;
            continue;
        }
    }

    return (curr == expected);
}

long encodePackBits (const unsigned char *ptr, const size_t size, unsigned char **output)
{
    unsigned char *out = NULL;

    if (*output == NULL)
    {
        out = (unsigned char *)malloc (size * 2 + 1); // max possible

        *output = out;
    }
    else
    {
        out = *output;
    }

    int repeat_count = 0;

    int literal_count = 0;

    int curr = 0;

    int literal_len_pos = -1;

    for (size_t i = 0; i <= size; )
    {
        int b = (i == size) ? -1 : ptr[i];

        repeat_count = 0;

        while (i < size - 1 && b == ptr[i + 1] && repeat_count < 128)
        {
            i++;
            repeat_count++;
        }

        i++;

        // two cases:
        // repeat count = 0: increase literal_len, assign literal_pos if not set.
        // repeat count > 0 :
        //    a) if literal_len > 0, flush literal_len, then write sequence
        //    b) if literal_len = 0 then just write sequence

        if (repeat_count == 0 && b >= 0)
        {
            if (literal_len_pos < 0)
            {
                literal_len_pos = curr;
                curr++;
            }

            out[curr++] = (unsigned char)b;

            literal_count++;

            if (literal_count == 128)
            {
                out[literal_len_pos] = (unsigned char)(literal_count - 1);
                literal_count = 0;
                literal_len_pos = -1;
            }
        }
        else
        {
            if (literal_count > 0)
            {
                out[literal_len_pos] = (unsigned char)(literal_count - 1);
                literal_len_pos = -1;
                literal_count = 0;
            }

            if (repeat_count > 0)
            {
                out[curr++] = (unsigned char)(127 + repeat_count);
                out[curr++] = (unsigned char)b;
            }

        }

    }

    return curr;
}


long getPackBitsSize (const unsigned char *ptr, const size_t size, long * limit)
{
    int repeat_count = 0;

    int literal_count = 0;

    long curr = 0;

    int literal_len_pos = -1;

    long m_limit = *limit ? *limit : (std::numeric_limits<long>::max)();

    for (size_t i = 0; i <= size; )
    {
        int b = (i == size) ? -1 : ptr[i];

        if (curr > m_limit)
            return -1;

        repeat_count = 0;

        while (i < size - 1 && b == ptr[i + 1] && repeat_count < 128)
        {
            i++;
            repeat_count++;
        }

        i++;

        // two cases:
        // repeat count = 0: increase literal_len, assign literal_pos if not set.
        // repeat count > 0 :
        //    a) if literal_len > 0, flush literal_len, then write sequence
        //    b) if literal_len = 0 then just write sequence

        if (repeat_count == 0 && b >= 0)
        {
            if (literal_len_pos < 0)
            {
                literal_len_pos = curr;
                curr++;
            }

            curr++;

            literal_count++;

            if (literal_count == 128)
            {
                literal_count = 0;
                literal_len_pos = -1;
            }
        }
        else
        {
            if (literal_count > 0)
            {
                literal_len_pos = -1;
                literal_count = 0;
            }

            if (repeat_count > 0)
            {
                curr += 2;
            }

        }

    }

    return curr;
}

enum HuffmanErrorCodes { MEMORY_ALLOC_FAIL = -1, HUFF_UNEXPECTED = -2 };

enum HuffmanFirstByte { HUFFMAN_NORMAL = 0, HUFFMAN_RLE = 1, HUFFMAN_NO_ENCODING = 2, HUFFMAN_PACKBITS = 3 };

bool ComputeHistoForHuffman(const unsigned char* data, size_t len, std::vector<int>& histo)
{
    histo.resize(256);

    memset (&histo[0], 0, histo.size() * sizeof(int));

    for (size_t i = 0; i < len; i++)
    {
        unsigned char val = data[i];

        histo[val]++;
    }

    int cnt = 0;
    for (size_t i = 0; i < 256; i++)
    {
        if (histo[i] > 0)
        {
            cnt++;
            if (cnt == 2) break;
        }
    }

    return (cnt > 1);
}

void ComputeHuffmanCodes(const unsigned char* data, size_t len, int& numBytes, std::vector<std::pair<unsigned short, unsigned int> >& codes)
{
    std::vector<int> histo;

    if (!ComputeHistoForHuffman(data, len, histo))
    {
        numBytes = -1;
        return;
    }

    int nBytes = 0;
    double avgBpp = 0;
    Huffman huffman;

    if (!huffman.ComputeCodes(histo) || !huffman.ComputeCompressedSize(histo, nBytes, avgBpp))
      nBytes = 0;

    if (nBytes > 0)
    {
        codes = huffman.GetCodes() ;
    }

    numBytes = nBytes;
}

int fpl_EsriHuffman::getCompressedSize (const char *input, size_t input_len)
{
    int numBytes = 0;
    std::vector<std::pair<unsigned short, unsigned int> > m_huffmanCodes;

    ComputeHuffmanCodes ((const unsigned char *)input, input_len, numBytes, m_huffmanCodes);

    if (numBytes < 0) return 6;

    if (numBytes == 0) return 0;

    if (numBytes > (int)input_len)
        return (int)input_len + 1;

    return numBytes + 1;
}

//////////////////////////////////////////////////////////////////////////////////
//  outputs size of encoded buffer.
//  input: byte array, its length;
//  output buffer, its length. If output len isn't sufficient, returns 0.
//////////////////////////////////////////////////////////////////////////////////

int fpl_EsriHuffman::EncodeHuffman (const char *input, size_t input_len, unsigned char ** ppByte, bool use_rle)
{
    int numBytes = 0;
    std::vector<std::pair<unsigned short, unsigned int> > m_huffmanCodes;

    ComputeHuffmanCodes ((const unsigned char *)input, input_len, numBytes, m_huffmanCodes);

    if (numBytes == -1)
    {
        // there's only one value in entire block (probably zeroes)
        // encode as 6 bytes:
        // byte 1 RLE flag
        // byte 2 repeating value
        // bytes 3-6 repeat count (32-bit unsigned int).
        *ppByte = (unsigned char *)calloc (6, 1);

        unsigned char *ptr = *ppByte;

        ptr[0] = HUFFMAN_RLE; // RLE flag
        ptr[1] = input[0];

        lerc_assert(input_len <= 0xffffffff);

        uint32_t len = (uint32_t)input_len;

        memcpy (ptr + 2, &len, 4);

        return 6;
    }

    if (numBytes == 0)
    {
        return HUFF_UNEXPECTED;
    }

    if (use_rle)
    {
        long limit = (std::min) (numBytes, (int)input_len);

        long rle_len = getPackBitsSize ((unsigned char *)input, input_len, &limit);
        if (rle_len > 0 && (rle_len < numBytes) && (rle_len < (long)input_len))
        {
            *ppByte = (unsigned char *)malloc (rle_len + 1);

            if (*ppByte == NULL)
              return MEMORY_ALLOC_FAIL;

            unsigned char * originalPtr = *ppByte ;
            originalPtr[0] = HUFFMAN_PACKBITS;

            unsigned char *packed = originalPtr + 1; //NULL;
            encodePackBits ((unsigned char *)input, input_len, &packed);

            return rle_len + 1;
        }
    }

    if (numBytes >= (int)input_len) // huffman will take more space than uncompressed. Don't encode.
    {
        *ppByte = (unsigned char *)malloc (input_len + 1);

        if (*ppByte == NULL)
          return MEMORY_ALLOC_FAIL;

        unsigned char * originalPtr = *ppByte ;
        originalPtr[0] = HUFFMAN_NO_ENCODING; // as is flag
        memcpy (originalPtr + 1, input, input_len);

        return (int)input_len + 1;
    }

    *ppByte = (unsigned char *)malloc (numBytes + 1);

    if (*ppByte == NULL)
      return MEMORY_ALLOC_FAIL;

    unsigned char * originalPtr = *ppByte ;

    originalPtr[0] = HUFFMAN_NORMAL; // normal "huffman flag"

    *ppByte = originalPtr + 1;

    Huffman huffman;

    if (!huffman.SetCodes(m_huffmanCodes) || !huffman.WriteCodeTable(ppByte, 5))    // header and code table
    {
        free (originalPtr);
        *ppByte = NULL;
        return HUFF_UNEXPECTED;
    }

    int bitPos = 0;
    int offset = 0;
    const unsigned char *data = (const unsigned char *)input;

    for (size_t m = 0; m < input_len; m++)
    {
        unsigned char val = data[m];

        // bit stuff the huffman code for this val
        int kBin = offset + (int)val;
        int len = m_huffmanCodes[kBin].first;
        if (len <= 0)
        {
            free (originalPtr);
            *ppByte = NULL;
            return HUFF_UNEXPECTED;
        }

        unsigned int code = m_huffmanCodes[kBin].second;

        if (!Huffman::PushValue(ppByte, bitPos, code, len))
        {
          free(originalPtr);
          *ppByte = NULL;
          return HUFF_UNEXPECTED;
        }
    }

    size_t numUInts = (bitPos > 0 ? 1 : 0) + 1;    // add one more as the decode LUT can read ahead
    *ppByte += numUInts * sizeof(unsigned int);

    int ret = (int)(*ppByte - originalPtr);

    *ppByte = originalPtr;

    return ret;
}

bool fpl_EsriHuffman::DecodeHuffman(const unsigned char* inBytes, const size_t inCount, size_t& nBytesRemainingInOut, unsigned char** output)
{
    const unsigned char* ppByte = (const unsigned char *)inBytes;

    if (!ppByte)
    {
        //fprintf (stderr, "bad input: null pointer\n");
        return false;
    }

    // check for RLE flag

    if (ppByte[0] == HUFFMAN_RLE)
    {
        unsigned char s = ppByte[1];
        uint32_t rle_size = 0;

        memcpy (&rle_size, ppByte + 2, 4);

        if (rle_size != nBytesRemainingInOut)
        {
            assert (0);
        }

        unsigned char *data = (unsigned char *)malloc (nBytesRemainingInOut);
        *output = data;

        if (data == NULL)
        {
            //fprintf (stderr, "malloc() failed\n");
            return false;
        }

        memset (data, s, nBytesRemainingInOut);

        return true;
    }

    if (ppByte[0] == HUFFMAN_NO_ENCODING) // "as is flag"
    {
        unsigned char *data = (unsigned char *)malloc (nBytesRemainingInOut);
        *output = data;

        if (data == NULL)
        {
            //printf ("malloc 2 fail\n");
            return false;
        }

        memcpy (data, ppByte + 1, nBytesRemainingInOut);

        return true;
    }

    if (ppByte[0] == HUFFMAN_PACKBITS)
    {
        unsigned char *unpacked = NULL;

        lerc_assert (true == decodePackBits (ppByte + 1, inCount - 1, nBytesRemainingInOut, &unpacked));

        *output = unpacked;

        return true;
    }

    assert (ppByte[0] == HUFFMAN_NORMAL); // "normal" huffman flag.

    ppByte++;

    size_t expected_output_len = nBytesRemainingInOut;

    Huffman huffman;
    if (!huffman.ReadCodeTable(&ppByte, nBytesRemainingInOut, 5))    // header and code table
    {
        return false;
    }

    int numBitsLUT = 0;
    if (!huffman.BuildTreeFromCodes(numBitsLUT))
    {
        return false;
    }

    unsigned char *data = (unsigned char *)malloc (expected_output_len);
    *output = data;

    if (!data)
    {
        return false;
    }

    int offset = 0;
    int bitPos = 0;
    size_t nBytesRemaining = nBytesRemainingInOut;

    for (size_t m = 0; m < expected_output_len; m++)
    {
        int val = 0;
        if (!huffman.DecodeOneValue(&ppByte, nBytesRemaining, bitPos, numBitsLUT, val))
        {
            return false;
        }

        data[m] = (unsigned char)(val - offset);
    }

    return true;
}
