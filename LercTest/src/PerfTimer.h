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

#pragma once

#include <windows.h>

// -------------------------------------------------------------------------- ;

/**	Windows performance timer. Much more accurate than clock(). 
 *	Resolution is ~1 us, compared to ~10 ms for clock().
 *
 *	MSDN:
 *	http://msdn.microsoft.com/en-us/library/ms644904(VS.85).aspx
 *
 *	Says that resolution is 1 us:
 *	http://search.cpan.org/~CRENZ/Win32-PerfCounter-0.02/PerfCounter.pm
 *
 *	Article about high precision time on Windows:
 *	http://msdn.microsoft.com/en-us/magazine/cc163996.aspx
 *
 */

class PerfTimer
{
public:

  PerfTimer();

  void start();
  void stop();           // cpu time for these fcts is ~0.1 us

  double sec() const;    // seconds, range is roughly [ 10^-9 ... 10^9 ]

  int ms() const;       // milli seconds (convenient: get ms or us as int)
  int us() const;       // micro seconds


protected:

  LARGE_INTEGER m_freq,   // counts per sec, usually cpu clock freq (~3GHz)
         m_begin, m_end;  // counter values filled at start() and stop()

  int convert(double factor) const;
};

// -------------------------------------------------------------------------- ;

inline PerfTimer::PerfTimer()
{
  QueryPerformanceFrequency(&m_freq);
  LARGE_INTEGER zero = {0}, mOne = {0, -1};
  m_begin = zero;
  m_end = mOne;
}

// -------------------------------------------------------------------------- ;

inline void PerfTimer::start()
{
  QueryPerformanceCounter(&m_begin);
}

// -------------------------------------------------------------------------- ;

inline void PerfTimer::stop()
{
  QueryPerformanceCounter(&m_end);
}

// -------------------------------------------------------------------------- ;

inline double PerfTimer::sec() const
{
  LONGLONG elapsed = m_end.QuadPart - m_begin.QuadPart;
  if (elapsed >= 0)
  {
    return elapsed / (double)m_freq.QuadPart;

    // as freq (1-3 GHz) is close to the max long, the result is in the 
    // range [ 1 / max_long ... max_long ] or [ 1 / billion ... billion ]
  }
  return -1;
}

// -------------------------------------------------------------------------- ;

inline int PerfTimer::ms() const
{
  return convert(1000);
}

// -------------------------------------------------------------------------- ;

inline int PerfTimer::us() const
{
  return convert(1000000);
}

// -------------------------------------------------------------------------- ;

inline int PerfTimer::convert(double fac) const
{
  double sec = this->sec();
  if (sec >= 0)
  {
    double d = sec * fac + 0.5;
    if (d <= 2147483647)           // max int
      return (int)d;
  }
  return -1;
}

// -------------------------------------------------------------------------- ;

