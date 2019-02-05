/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "placid"

#include "bare/Serial.h"

using namespace placid;

int32_t Placid::vprintf(const char* format, va_list args)
{
    // FIXME: Eventually this needs to be a system call
    return bare::Serial::vprintf(format, args);
}
