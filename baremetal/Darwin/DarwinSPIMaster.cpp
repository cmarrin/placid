/*-------------------------------------------------------------------------
    This source file is a part of Placid

    For the latest info, see http:www.marrin.org/

    Copyright (c) 2018-2019, Chris Marrin
    All rights reserved.

    Use of this source code is governed by the MIT license that can be
    found in the LICENSE file.
-------------------------------------------------------------------------*/

#include "bare.h"

#include "bare/SPIMaster.h"

#include "bare/Serial.h"

using namespace bare;

static void showSim(const char* format, ...)
{
    va_list args;
    va_start(args, format);
    Serial::printf("[SPIMaster Sim]===> ");
    Serial::vprintf(format, args);
    va_end(args);
}

void SPIMaster::init(uint32_t transferRate, EnablePolarity, ClockEdge, ClockPolarity)
{
    showSim("init\n");
}

int32_t SPIMaster::readWrite(char* readBuf, const char* writeBuf, int32_t size)
{
    showSim("readWrite %d bytes:");
    if (writeBuf) {
        for (int i = 0; i < size; ++i) {
            Serial::printf("%02x", writeBuf[i]);
        }
    }
    Serial::printf("\n");
    return size;
}

void SPIMaster::startTransfer()
{
    showSim("startTransfer\n");
}

uint32_t SPIMaster::transferByte(uint8_t b)
{
    showSim("transferByte(0x%02x)\n", b);
    return AnyByte;
}

void SPIMaster::endTransfer()
{
    showSim("endTransfer\n");
}

bool SPIMaster::simulatedData()
{
    return true;
}
