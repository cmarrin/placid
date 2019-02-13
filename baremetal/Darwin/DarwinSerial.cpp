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
#include <unistd.h>
#include <errno.h>
#include <util.h>
#include <termios.h>

//#define USE_PTY

using namespace bare;

#ifdef USE_PTY
static int master;
static int slave;
#endif

void Serial::init(uint32_t baudrate)
{
#ifdef USE_PTY
    struct termios tty;
    tty.c_iflag = (tcflag_t) 0;
    tty.c_lflag = (tcflag_t) 0;
    tty.c_cflag = CS8;
    tty.c_oflag = (tcflag_t) 0;

    char buf[256];

    auto e = openpty(&master, &slave, buf, &tty, nullptr);
    if(0 > e) {
    ::printf("Error: %s\n", strerror(errno));
        return;
    }

    ::printf("Slave PTY: %s\n", buf);
    fflush(stdout);
#endif
}

Serial::Error Serial::read(uint8_t& c)
{
#ifdef USE_PTY
    return (::read(master, &c, 1) == -1) ? Error::Fail : Error::OK;
#else
    c = getchar();
    return Error::OK;
#endif
}

bool Serial::rxReady()
{
    return true;
}

Serial::Error Serial::write(uint8_t c)
{
#ifdef USE_PTY
    ::write(master, &c, 1);
#else
    if (c != '\r') {
        std::cout.write(reinterpret_cast<const char*>(&c), 1);
    }
#endif
    return Error::OK;
}

void Serial::handleInterrupt()
{
}

void Serial::clearInput() 
{
}
