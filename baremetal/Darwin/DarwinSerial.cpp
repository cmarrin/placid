/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/Serial.h"

#include "bare/GPIO.h"
#include "bare/InterruptManager.h"
#include "bare/Timer.h"

#include <iostream>

using namespace bare;

void Serial::init(uint32_t baudrate)
{
    //system("stty raw");
}

Serial::Error Serial::read(uint8_t& c)
{
    c = getchar();
    return Error::OK;
}

bool Serial::rxReady()
{
    return true;
}

Serial::Error Serial::write(uint8_t c)
{
    std::cout.write(reinterpret_cast<const char*>(&c), 1);
    return Error::OK;
}

void Serial::handleInterrupt()
{
}

void Serial::clearInput() 
{
}
