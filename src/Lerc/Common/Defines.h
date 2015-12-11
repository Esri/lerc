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

typedef unsigned char Byte;

#ifndef max
#define max(a,b)      (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)      (((a) < (b)) ? (a) : (b))
#endif

// drop support for big endian in Lerc2
#define SWAP_2(x)
#define SWAP_4(x)
#define SWAP_8(x)

