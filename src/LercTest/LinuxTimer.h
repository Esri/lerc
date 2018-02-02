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

#include <time.h>

// see also  http://tdistler.com/2010/06/27/high-performance-timing-on-linux-windows

class PerfTimer
{
public:

  PerfTimer();

  void start();
  void stop();

  double sec() const;    // seconds, range is roughly [ 10^-9 ... 10^9 ]

  int ms() const;       // milli seconds (convenient: get ms or us as int)
  int us() const;       // micro seconds

protected:
  struct timespec m_res, m_begin, m_end;

  void ask(int& sec, int& nsec) const;
};

// -------------------------------------------------------------------------- ;

inline PerfTimer::PerfTimer()
{
  clock_getres( CLOCK_MONOTONIC, &m_res);
  clock_gettime(CLOCK_MONOTONIC, &m_begin);
  clock_gettime(CLOCK_MONOTONIC, &m_end);
}

// -------------------------------------------------------------------------- ;

inline void PerfTimer::start()
{
  clock_gettime(CLOCK_MONOTONIC, &m_begin);
}

// -------------------------------------------------------------------------- ;

inline void PerfTimer::stop()
{
  clock_gettime(CLOCK_MONOTONIC, &m_end);
}

// -------------------------------------------------------------------------- ;

inline double PerfTimer::sec() const
{
  int sec, nsec;
  ask(sec, nsec);
  return sec + (double)nsec / 1000000000;
}

// -------------------------------------------------------------------------- ;

inline int PerfTimer::ms() const
{
  int sec, nsec;
  ask(sec, nsec);
  return sec * 1000 + (double)nsec / 1000000;
}

// -------------------------------------------------------------------------- ;

inline int PerfTimer::us() const
{
  int sec, nsec;
  ask(sec, nsec);
  return sec * 1000000 + (double)nsec / 1000;
}

// -------------------------------------------------------------------------- ;

inline void PerfTimer::ask(int& sec, int& nsec) const
{
  nsec = m_end.tv_nsec - m_begin.tv_nsec;
  nsec += nsec < 0 ? 1000000000 : 0;
  sec = m_end.tv_sec - m_begin.tv_sec - (nsec < 0) ? 1 : 0;
}

// -------------------------------------------------------------------------- ;

