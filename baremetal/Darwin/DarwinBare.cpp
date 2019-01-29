/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include <stdio.h>
#include <sys/time.h>

using namespace bare;

static bool inited = false;

void bare::initSystem()
{
    inited = true;
}

bool bare::useAllocator()
{
    return inited;
}

// Setup a dummy area of "kernel space" to dump data to
uint8_t _dummyKernel[8 * 1024 * 1024];

extern "C" {

uint8_t* kernelBase() { return _dummyKernel; }

void PUT8(uint8_t* addr, uint8_t value)
{
    printf("PUT8:[0x%p] <= %#02x\n", addr, value);
}

void BRANCHTO(uint8_t* addr)
{
    printf("BRANCHTO: => 0x%p\n", addr);
    while (1) ;
}

bool interruptsSupported()
{
    return false;
}

void reboot()
{
    printf("REBOOT\n");
    while (1) ;
}

}
