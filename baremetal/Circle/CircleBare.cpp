/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include <circle/startup.h>

using namespace bare;

void bare::initSystem()
{
}

bool bare::interruptsSupported()
{
    return true;
}

bool bare::useAllocator()
{
    return true;
}
