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

#include "bare/Serial.h"

#include "bare/Serial.h"
#include "Serial.h"

using namespace bare;

void Serial::init(uint32_t baudrate)
{
    ::Serial.begin(baudrate);
}

Serial::Error Serial::read(uint8_t& c)
{
    int b = ::Serial.read();
    if (b < 0) {
        c = 0;
        return Error::NotReady;
    }
    c = static_cast<uint8_t>(b);
	return Error::OK;
}

bool Serial::rxReady()
{
    return ::Serial.available() > 0;
}

Serial::Error Serial::write(uint8_t c)
{
    return (::Serial.write(c) == 1) ? Error::OK : Error::Fail;
}

void Serial::handleInterrupt()
{
}

void Serial::clearInput() 
{
}
