/*
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

Contributors:  Thomas Maurer
*/

#ifndef CNTZIMAGE_H
#define CNTZIMAGE_H

#include <vector>
#include "TImage.hpp"

NAMESPACE_LERC_START

/**	count / z image
 *
 *	count can also be a weight, therefore float;
 *	z can be elevation or intensity;
 */

class CntZImage : public TImage< CntZ >
{
public:
  CntZImage();
  virtual ~CntZImage()  {};
  std::string getTypeString() const  { return "CntZImage "; }

  bool resizeFill0(int width, int height);

  static unsigned int computeNumBytesNeededToReadHeader();

  /// read succeeds only if maxZError on file <= maxZError requested
  bool read(Byte** ppByte, bool& hasInvalidData, double maxZError, bool onlyHeader = false, bool onlyZPart = false);

  template <class T>
  bool ConvertToMemBlock(T* arr, T noDataValue) const;

protected:

  struct InfoFromComputeNumBytes
  {
    double maxZError;
    bool cntsNoInt;
    int numTilesVertCnt;
    int numTilesHoriCnt;
    int numBytesCnt;
    float maxCntInImg;
    int numTilesVertZ;
    int numTilesHoriZ;
    int numBytesZ;
    float maxZInImg;
  };

  bool readTiles(bool zPart, double maxZErrorInFile, int numTilesVert, int numTilesHori, float maxValInImg, Byte* bArr);

  bool readCntTile(Byte** ppByte, int i0, int i1, int j0, int j1);
  bool readZTile(Byte** ppByte, int i0, int i1, int j0, int j1, double maxZErrorInFile, float maxZInImg);

  static int numBytesFlt(float z);    // returns 1, 2, or 4
  static bool readFlt(Byte** ppByte, float& z, int numBytes);

protected:

  InfoFromComputeNumBytes    m_infoFromComputeNumBytes;
  std::vector<unsigned int>  m_tmpDataVec;             // used in read fcts
  bool                       m_bDecoderCanIgnoreMask;  // "
};

template <class T>
bool CntZImage::ConvertToMemBlock(T* arr, T noDataValue) const
{
  if (!arr)
  {
    return false;
  }

  const CntZ* srcPtr = getData();
  T* dstPtr = arr;

  for (int32_t i = 0; i < height_; i++)
  {
    for (int32_t j = 0; j < width_; j++)
    {
      if (srcPtr->cnt > 0)
      {
        *dstPtr++ = (T)srcPtr->z;
      }
      else
      {
        *dstPtr++ = noDataValue;
      }
      srcPtr++;
    }
  }

  return true;
}

NAMESPACE_LERC_END
#endif
