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

