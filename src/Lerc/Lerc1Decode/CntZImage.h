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

  http://www.github.com/esri/lerc/LICENSE
  http://www.github.com/esri/lerc/NOTICE

Contributors:  Thomas Maurer
*/

#pragma once

#include "TImage.hpp"
#include "BitStuffer.h"

/**	count / z image
*
*	count can also be a weight, therefore float;
*	z can be elevation or intensity;
*/

namespace LercNS
{
  class CntZImage : public TImage< CntZ >
  {
  public:
    CntZImage();
    virtual ~CntZImage();
    std::string getTypeString() const    { return "CntZImage "; }

    bool resizeFill0(int width, int height);

    /// read succeeds only if maxZError on file <= maxZError requested (!)
    static unsigned int numExtraBytesToAllocate()    { return BitStuffer::numExtraBytesToAllocate(); }
    static unsigned int computeNumBytesNeededToReadHeader();

    bool read(Byte** ppByte,
      double maxZError,
      bool onlyHeader = false,
      bool onlyZPart = false);

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

    bool readTiles(bool zPart, double maxZErrorInFile,
      int numTilesVert, int numTilesHori, float maxValInImg, Byte* bArr);

    bool readCntTile(Byte** ppByte, int i0, int i1, int j0, int j1);
    bool readZTile(Byte** ppByte, int i0, int i1, int j0, int j1, double maxZErrorInFile, float maxZInImg);

    int numBytesFlt(float z) const;    // returns 1, 2, or 4
    bool readFlt(Byte** ppByte, float& z, int numBytes) const;

  protected:

    int                        m_currentVersion;
    InfoFromComputeNumBytes    m_infoFromComputeNumBytes;

    std::vector<unsigned int>  m_tmpDataVec;             // used in read fcts
    bool                       m_bDecoderCanIgnoreMask;  // "
  };

}

