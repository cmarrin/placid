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

#define _IRQStack   0x1000
#define _FIQStack   0x2000
#define _AbortStack 0x3000
#define _SVCStack   0x100000

#define EXCEPTION_DIVISION_BY_ZERO      0
#define EXCEPTION_UNDEFINED_INSTRUCTION 1
#define EXCEPTION_PREFETCH_ABORT        2
#define EXCEPTION_DATA_ABORT            3
#define EXCEPTION_UNKNOWN               4
