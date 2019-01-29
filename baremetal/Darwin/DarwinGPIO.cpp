/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/GPIO.h"

using namespace bare;

void GPIO::setFunction(uint32_t pin, Function f, Pull)
{
}

void GPIO::setPin(uint32_t pin, bool on)
{
}

bool GPIO::getPin(uint32_t pin)
{
    return false;
}

volatile uint32_t& GPIO::reg(Register r)
{
    static uint32_t _dummy = 0;
    return _dummy;
}
