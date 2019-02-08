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

using namespace bare;

bool bare::receiveFile(ReceiveFunction func)
{
    Serial::printf("Start X/YModem download when ready, or press [esc] key to cancel...\n");
    Serial::printf("    (Use YModem to accurately set file size)\n");

    XYModem xyModem(
        [](uint8_t& c) { bare::Serial::read(c); },
        [](uint8_t c) { bare::Serial::write(c); },
        []() -> bool { return bare::Serial::rxReady(); },
        []() -> uint32_t { return static_cast<uint32_t>(bare::Timer::systemTime() / 1000); });

    return !xyModem.receive(func);
}
