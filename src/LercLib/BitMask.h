/*
Copyright 2015 - 2026 Esri

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

#ifndef BITMASK_H
#define BITMASK_H

#include "Defines.h"
#include <cstddef>
#include <cstdint>

NAMESPACE_LERC_START

/** BitMask - Convenient and fast access to binary mask bits
*
*/

class BitMask
{
public:
  BitMask() : m_pBits(nullptr), m_nCols(0), m_nRows(0)  {}
  BitMask(int nCols, int nRows) : m_pBits(nullptr), m_nCols(0), m_nRows(0) { SetSize(nCols, nRows); }
  BitMask(const BitMask& src);
  ~BitMask()                                { Clear(); }

  BitMask& operator= (const BitMask& src);

  // 1: valid, 0: not valid
  Byte IsValid(int64_t k) const             { return (m_pBits[k >> 3] & Bit(k)) > 0; }
  Byte IsValid(int row, int col) const      { return IsValid((int64_t)row * m_nCols + col); }

  void SetValid(int64_t k) const            { m_pBits[k >> 3] |= Bit(k); }
  void SetValid(int row, int col) const     { SetValid((int64_t)row * m_nCols + col); }

  void SetInvalid(int64_t k) const          { m_pBits[k >> 3] &= ~Bit(k); }
  void SetInvalid(int row, int col) const   { SetInvalid((int64_t)row * m_nCols + col); }

  void SetAllValid() const;
  void SetAllInvalid() const;

  bool SetSize(int nCols, int nRows);

  int GetWidth() const                      { return m_nCols; }
  int GetHeight() const                     { return m_nRows; }
  size_t Size() const                       { int64_t n = ((int64_t)m_nCols * m_nRows + 7) >> 3; return n >= 0 ? (size_t)n : 0; }
  const Byte* Bits() const                  { return m_pBits; }
  Byte* Bits()                              { return m_pBits; }
  static Byte Bit(int64_t k)                { return (1 << 7) >> (k & 7); }

  int64_t CountValidBits() const;
  void Clear();

private:
  Byte*  m_pBits;
  int    m_nCols, m_nRows;
};

NAMESPACE_LERC_END

#endif
