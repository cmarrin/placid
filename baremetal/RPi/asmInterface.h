/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

// This file is shared between assembly and c/c++

#pragma once

// Memory mapping
//
// Kernal occupies the first 1 MB of physical memory, so it occupies the
// first TTB. This first TTB has a second level translation table which
// maps 256 4KB pages. These pages are used as follows:
//
//      0           0x0000  - 0x0800    reserved for system use (interrupt vectors, ATAGS, etc.)
//                  0x0800  - 0x0c00    second level translation table.
//                  0x0c00  - 0x1000    unused
//
//      1           0x1000  - 0x2000    unused
//      2           0x2000  - 0x3000    unused
//      3           0x3000  - 0x4000    unused
//      4 - 7       0x4000  - 0x8000    first level translation table
//      8 - ???     0x8000  - _end      kernel code, rodata and bss
//      ??? - 250   _end    - 0xfafff   heap at the bottom and the system stack at the top (2)
//      251         0xfb000 - 0xfbfff   FIQ stack
//      252         0xfc000 - 0xfcfff   IRQ stack
//      253         0xfd000 - 0xfdfff   Abort stack
//      254         0xfe000 - 0xfefff   Undefined instruction stack
//      255         0xff000 - 0xfffff   SVC stack
//
//      1) The interrupt stacks occupy one 4096 byte page. If a page fault occurs for any, a 
//      kernel panic is generated
//
//      2) SVC stack and heap occupy whole pages (4096 bytes) if a page fault occurs in either and
//      the adjacent page is allocated to the other, a kernel panic is generated
//

#define _SystemStack    0xfb000
#define _FIQStack       0xfc000
#define _IRQStack       0xfd000
#define _AbortStack     0xfe000
#define _UndefinedStack 0xff000
#define _SVCStack       0x100000

#define EXCEPTION_DIVISION_BY_ZERO      0
#define EXCEPTION_UNDEFINED_INSTRUCTION 1
#define EXCEPTION_PREFETCH_ABORT        2
#define EXCEPTION_DATA_ABORT            3
#define EXCEPTION_FIQ                   4
#define EXCEPTION_UNKNOWN               5
