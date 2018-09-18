/*-------------------------------------------------------------------------
This source file is a part of Placid

For the latest info, see http://www.marrin.org/

Copyright (c) 2018, Chris Marrin
All rights reserved.

Redistribution and use in source and binary forms, with or without 
modification, are permitted provided that the following conditions are met:

    - Redistributions of source code must retain the above copyright notice, 
    this list of conditions and the following disclaimer.
    
    - Redistributions in binary form must reproduce the above copyright 
    notice, this list of conditions and the following disclaimer in the 
    documentation and/or other materials provided with the distribution.
    
    - Neither the name of the <ORGANIZATION> nor the names of its 
    contributors may be used to endorse or promote products derived from 
    this software without specific prior written permission.
    
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" 
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE 
LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
POSSIBILITY OF SUCH DAMAGE.
-------------------------------------------------------------------------*/

#include "BootShell.h"

#include "Serial.h"
#include "Timer.h"
#include "xmodem.h"

using namespace placid;

extern "C" int _inbyte(unsigned short timeout)
{
    // Timeout is in ms
    int8_t c;
    if (Serial::read(c, timeout) == Serial::Error::OK) {
        return c;
    } else {
        return -1;
    }
}

extern "C" void _outbyte(int c)
{
    Serial::write(static_cast<int8_t>(c));
}

static char _testdata[ ] =
    "\x01\x01\xfe"  // SOH, block number, inverse block number
    "0123456789"    // Data
    "0123456789"
    "0123456789"
    "0123456789"
    "0123456789"
    "0123456789"
    "0123456789"
    "0123456789"
    "0123456789"
    "0123456789"
    "0123456789"
    "0123456789"
    "0123456789"
    "\xC1\xc2"      // Checksum
    "\04";          // EOT

static uint32_t _testindex;

extern "C" int _testinbyte(unsigned short timeout)
{
    return (_testindex >= 136) ? -1 : static_cast<uint8_t>(_testdata[_testindex++]);
}

extern "C" void _testoutbyte(int c)
{
    placid::cout << "test xmodem received '" << static_cast<char>(c) << "'\n";
}

extern "C" void _xmodemmemcpy(unsigned char *dst, unsigned char *src, int count)
{
#ifdef __APPLE__
    placid::cout << "*** placing " << count << " bytes at " << static_cast<uint32_t>(reinterpret_cast<uint64_t>(dst)) << "\n";
#else
    for (int i = 0; i < count; i++) {
        PUT8(reinterpret_cast<uint32_t>(dst++), *src++);
    }
#endif
}

const char* BootShell::welcomeString() const
{
	return "\n\nWelcome to The Placid Boot Shell v0.1";
}

const char* BootShell::helpString() const
{
	return "Commands:\n    '?' : this help message\n    'l' : load executable using XModem";
} 

void BootShell::shellSend(const char* data, uint32_t size)
{
	Serial::puts(data, size);
}

void showXModemError(int error) {
    if (error > 0) {
        placid::cout << "XModem transfer successful, " << static_cast<uint32_t>(error) << " bytes transferred\n";
        return;
    }
    const char* errorString;
    switch (error) {
    case -1: errorString = "canceled by remote"; break;
    case -2: errorString = "sync error"; break;
    case -3: errorString = "canceled by remote"; break;
    default: errorString = "too many retry"; break;
    }
    
    placid::cout << "XModem transfer FAILED: " << errorString << "\n";
}

bool BootShell::executeShellCommand(const char* s)
{
    if (s[0] == 'l') {
#ifdef __APPLE__
#else
        placid::cout << "Waiting for XModem transfer to start...\n";
        Timer::delay(0.5);
        int error = xmodemReceive(0x8000, 1024 * 1024);
        showXModemError(error);
#endif
        return true;
    }
    return false;
}

