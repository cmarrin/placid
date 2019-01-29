/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#pragma once

#include "Serial.h"

static inline void ERROR_LOG(const char* format, ...)
{
    bare::Serial::printf("ERROR: ");
    va_list va;
    va_start(va, format);
    bare::Serial::vprintf(format, va);
    va_end(va);
}
#ifdef ENABLE_DEBUG_LOG
static inline void DEBUG_LOG(const char* format, ...)
{
    bare::Serial::printf("log  : ");
    va_list va;
    va_start(va, format);
    bare::Serial::vprintf(format, va);
    va_end(va);
}
#else
static inline void DEBUG_LOG(const char* format, ...) { }
#endif

#undef ENABLE_DEBUG_LOG
