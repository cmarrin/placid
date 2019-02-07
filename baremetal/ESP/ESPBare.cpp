/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include <Arduino.h>

#include "bare.h"

#include <stdio.h>
#include <sys/time.h>

using namespace bare;

void bare::initSystem()
{
}

bool bare::useAllocator()
{
    return true;
}

extern "C" {

uint8_t* kernelBase() { return nullptr; }

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

void restart()
{
    ESP.restart();
}

}
