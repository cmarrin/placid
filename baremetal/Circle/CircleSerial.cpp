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
#include <circle/serial.h>

using namespace bare;

CSerialDevice g_serial;
int32_t g_pushback = -1;

void Serial::init(uint32_t baudrate)
{
    baudrate = std::min(std::max(static_cast<int>(baudrate), 110), 31250000);
	g_serial.Initialize(baudrate);
}

Serial::Error Serial::read(uint8_t& c)
{
	if (g_pushback >= 0) {
		c = g_pushback;
		g_pushback = -1;
		return Error::OK;
	}
	
	while(true) {
		int result = g_serial.Read(&c, 1);
		switch(result) {
			case 0: continue;
			case 1: return Error::OK;
			case SERIAL_ERROR_BREAK: return Error::Break;
			case SERIAL_ERROR_OVERRUN: return Error::Overrun;
			case SERIAL_ERROR_FRAMING: return Error::Framing;
			case SERIAL_ERROR_PARITY: return Error::Parity;
			default: return Error::Fail;
		}
	}
}

bool Serial::rxReady()
{
	if (g_pushback >= 0) {
		return true;
	}
	
	uint8_t c;
	int result = g_serial.Read(&c, 1);
	if (result == 1) {
		g_pushback = c;
		return true;
	}
	
	return false;
}

Serial::Error Serial::write(uint8_t c)
{
	int result = g_serial.Write(&c, 1);
	switch(result) {
		case 1: return Error::OK;
		case SERIAL_ERROR_BREAK: return Error::Break;
		case SERIAL_ERROR_OVERRUN: return Error::Overrun;
		case SERIAL_ERROR_FRAMING: return Error::Framing;
		case SERIAL_ERROR_PARITY: return Error::Parity;
		default: return Error::Fail;
	}
}

void Serial::clearInput() 
{
	g_pushback = -1;
}
