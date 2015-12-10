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

#include "Image.h"
#include <vector>
#include "../Common/Defines.h"

namespace LercNS
{
  class CntZ
  {
  public:
    float cnt, z;
    bool operator == (const CntZ& cz) const    { return cnt == cz.cnt && z == cz.z; }
    bool operator != (const CntZ& cz) const    { return cnt != cz.cnt || z != cz.z; }
    void operator += (const CntZ& cz)          { cnt += cz.cnt;  z += cz.z; }
  };

  template< class Element >
  class TImage : public Image
  {
  public:
    TImage();
    TImage(const TImage& tImg);
    virtual ~TImage();

    /// assignment
    virtual TImage& operator=(const TImage& tImg);

    bool resize(int width, int height);
    void fill(Element value);
    void fillRow(int row, Element val);
    void fillCol(int col, Element val);
    virtual void clear();

    /// get data
    Element getPixel(int row, int col) const;
    const Element& operator() (int row, int col) const;
    const Element* getData() const;
    int getSizeInBytes() const;
    bool getColumn(int col, std::vector< Element >& colVec) const;
    bool getRange(Element& min, Element& max) const;
    bool getRangeFromValidData(Element& min, Element& max, Element invalidValue) const;

    /// set data
    void setPixel(int row, int col, Element element);
    Element& operator() (int row, int col);
    Element* getData();
    bool setColumn(int col, const std::vector< Element >& colVec);

    /// compare
    bool operator == (const Image& img) const;
    bool operator != (const Image& img) const	{ return !operator==(img); };

  protected:
    Element* data_;

  };

  // -------------------------------------------------------------------------- ;
  // -------------------------------------------------------------------------- ;

  template< class Element >
  inline Element TImage< Element >::getPixel(int i, int j) const
  {
    return data_[i * width_ + j];
  }

  template< class Element >
  inline const Element& TImage< Element >::operator () (int i, int j) const
  {
    return data_[i * width_ + j];
  }

  template< class Element >
  inline const Element* TImage< Element >::getData() const
  {
    return data_;
  }

  template< class Element >
  inline int TImage< Element >::getSizeInBytes() const
  {
    return (getSize() * sizeof(Element) + sizeof(*this));
  }

  template< class Element >
  inline void TImage< Element >::setPixel(int i, int j, Element element)
  {
    data_[i * width_ + j] = element;
  }

  template< class Element >
  inline Element& TImage< Element >::operator () (int i, int j)
  {
    return data_[i * width_ + j];
  }

  template< class Element >
  inline Element* TImage< Element >::getData()
  {
    return data_;
  }

  // -------------------------------------------------------------------------- ;
  // -------------------------------------------------------------------------- ;

  template< class Element >
  TImage< Element >::TImage()
  {
    data_ = 0;
  };

  template< class Element >
  TImage< Element >::TImage(const TImage& tImg)
  {
    data_ = 0;
    *this = tImg;
  };

  template< class Element >
  TImage< Element >::~TImage()
  {
    clear();
  };

  // -------------------------------------------------------------------------- ;

  template< class Element >
  bool TImage< Element >::resize(int width, int height)
  {
    if (width <= 0 || height <= 0)
      return false;

    if (width == width_ && height == height_)
      return true;

    free(data_);
    width_ = 0;
    height_ = 0;

    data_ = (Element*)malloc(width * height * sizeof(Element));
    if (!data_)
      return false;

    width_ = width;
    height_ = height;

    return true;
  }

  // -------------------------------------------------------------------------- ;

  template< class Element >
  void TImage< Element >::fill(Element val)
  {
    Element* ptr = getData();
    int size = getSize();

    int i = size >> 2;
    while (i--)
    {
      *(ptr) = val;
      *(ptr + 1) = val;
      *(ptr + 2) = val;
      *(ptr + 3) = val;
      ptr += 4;
    }
    i = size & 3;
    while (i--) *ptr++ = val;
  }

  // -------------------------------------------------------------------------- ;

  template< class Element >
  void TImage< Element >::fillRow(int row, Element val)
  {
    if (row < 0 || row >= height_)
      return;

    Element* ptr = getData() + row * width_;
    int size = width_;

    int i = size >> 2;
    while (i--)
    {
      *(ptr) = val;
      *(ptr + 1) = val;
      *(ptr + 2) = val;
      *(ptr + 3) = val;
      ptr += 4;
    }
    i = size & 3;
    while (i--) *ptr++ = val;
  }

  // -------------------------------------------------------------------------- ;

  template< class Element >
  void TImage< Element >::fillCol(int col, Element val)
  {
    if (col < 0 || col >= width_)
      return;

    Element* ptr = getData() + col;
    int i = height_;

    while (i--)
    {
      *ptr = val;
      ptr += width_;
    }
  }

  // -------------------------------------------------------------------------- ;

  template< class Element >
  void TImage< Element >::clear()
  {
    free(data_);
    data_ = 0;
    width_ = 0;
    height_ = 0;
  }

  // -------------------------------------------------------------------------- ;

  template< class Element >
  bool TImage< Element >::getColumn(int col, std::vector< Element >& colVec) const
  {
    colVec.resize(height_);

    for (int i = 0; i < height_; i++)
      colVec[i] = data_[i * width_ + col];

    return true;
  }

  // -------------------------------------------------------------------------- ;

  template< class Element >
  bool TImage< Element >::getRange(Element& min, Element& max) const
  {
    const Element* ptr = getData();
    int size = getSize();
    if (size == 0 || ptr == 0)
      return false;

    Element minL = *ptr;
    Element maxL = *ptr;
    int cnt = size;
    while (cnt--)
    {
      Element val = *ptr++;
      minL = val < minL ? val : minL;
      maxL = val > maxL ? val : maxL;
    }

    min = minL;
    max = maxL;

    return true;
  }

  // -------------------------------------------------------------------------- ;

  template< class Element >
  bool TImage< Element >::getRangeFromValidData(Element& min, Element& max, Element invalidValue) const
  {
    const Element* ptr = getData();
    int size = getSize();
    if (size == 0 || ptr == 0)
      return false;

    Element minL = *ptr;
    Element maxL = *ptr;
    bool init = false;
    int cnt = size;
    while (cnt--)
    {
      Element val = *ptr++;
      if (val != invalidValue)
      {
        if (!init)
        {
          minL = val;
          maxL = val;
          init = true;
        }
        else
        {
          minL = val < minL ? val : minL;
          maxL = val > maxL ? val : maxL;
        }
      }
    }

    min = minL;
    max = maxL;

    return true;
  }

  // -------------------------------------------------------------------------- ;

  template< class Element >
  bool TImage< Element >::setColumn(int col, const std::vector< Element >& colVec)
  {
    for (int i = 0; i < height_; i++)
      data_[i * width_ + col] = colVec[i];

    return true;
  }

  // -------------------------------------------------------------------------- ;

  template< class Element >
  TImage< Element >& TImage< Element >::operator = (const TImage& tImg)
  {
    // allow copying image to itself
    if (this == &tImg) return *this;

    // only for images of the same type!
    // conversions are implemented in the derived classes

    if (!resize(tImg.getWidth(), tImg.getHeight()))
      return *this;    // return empty image if resize fails

    memcpy(getData(), tImg.getData(), getSize() * sizeof(Element));

    Image::operator=(tImg);

    return *this;
  }

  // -------------------------------------------------------------------------- ;

  template< class Element >
  bool TImage< Element >::operator == (const Image& img) const
  {
    if (!Image::operator == (img)) return false;

    const Element* ptr0 = getData();
    const Element* ptr1 = ((const TImage&)img).getData();
    int cnt = getSize();
    while (cnt--)
      if (*ptr0++ != *ptr1++)
        return false;

    return true;
  }

  // -------------------------------------------------------------------------- ;
  // -------------------------------------------------------------------------- ;

  class DoubleImage : public TImage< double >
  {
  public:
    DoubleImage()                        { type_ = DOUBLE; }
    virtual ~DoubleImage()               {};
    std::string getTypeString() const    { return "DoubleImage "; }
  };

  // -------------------------------------------------------------------------- ;

  class FloatImage : public TImage< float >
  {
  public:
    FloatImage()                         { type_ = FLOAT; }
    virtual ~FloatImage()                {};
    std::string getTypeString() const    { return "FloatImage "; }
  };

  // -------------------------------------------------------------------------- ;

  class LongImage : public TImage< long >
  {
  public:
    LongImage()                          { type_ = LONG; }
    virtual ~LongImage()                 {};
    std::string getTypeString() const    { return "LongImage "; }
  };

  // -------------------------------------------------------------------------- ;

  class ShortImage : public TImage< short >
  {
  public:
    ShortImage()                         { type_ = SHORT; }
    virtual ~ShortImage()                {};
    std::string getTypeString() const    { return "ShortImage "; }
  };

  // -------------------------------------------------------------------------- ;

  class ByteImage : public TImage< Byte >
  {
  public:
    ByteImage()                          { type_ = BYTE; }
    virtual ~ByteImage()                 {};
    std::string getTypeString() const    { return "ByteImage "; }
  };
}
