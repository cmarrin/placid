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

#include "bare.h"

#include "bare/XYModem.h"

using namespace bare;

static constexpr uint32_t SOH = 0x01;
static constexpr uint32_t ACK = 0x06;
static constexpr uint32_t NAK = 0x15;
static constexpr uint32_t EOT = 0x04;
static constexpr uint32_t CAN = 0x18;

//#define CAPTURE_DATA
//#define TEST_XMODEM
#define TEST_XMODEM_HEADER

#ifdef TEST_XMODEM
static const char HeaderBlock[] = 
    "\x01\x00\xff"
    "FileSystem.d\0"
    "187 13364527115 100644 0 1 187"
    "\0\0\0\0\0\0\0\0\0\0"
    "\0\0\0\0\0\0\0\0\0\0"
    "\0\0\0\0\0\0\0\0\0\0"
    "\0\0\0\0\0\0\0\0\0\0"
    "\0\0\0\0\0\0\0\0\0\0"
    "\0\0\0\0\0\0\0\0\0\0"
    "\0\0\0\0\0\0\0\0\0\0"
    "\0\0\0\0\0\0\0\0\0\0"
    "\0\0\0\0"
    "\x02\x3f"
;

static_assert(sizeof(HeaderBlock) == 133, "Wrong Size HeaderBlock");

static const char DataBlock0[] = 
    "\x01\x01\xfe"
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
    "01234567"
    "\x38"
;

static_assert(sizeof(DataBlock0) == 133, "Wrong Size DataBlock0");

static const char DataBlock1[] = 
    "\x01\x02\xfd"
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
    "01234567"
    "\x38"
;

static_assert(sizeof(DataBlock1) == 133, "Wrong Size DataBlock1");

#endif

#ifdef CAPTURE_DATA
char _buffer[512];
uint32_t _bufferIndex;

void showXModemData()
{
    bare::Serial::printf("\n\nxmodem buffer:\n");
    for (uint32_t i = 0; i < _bufferIndex; ++i) {
        bare::Serial::printf("    0x%02x\n", _buffer[i]);
    }
    bare::Serial::printf("\n\n");
}
#endif

bool XYModem::receive(ReceiveFunction func)
{
#ifdef CAPTURE_DATA
    _bufferIndex = 0;
#endif

#ifdef TEST_XMODEM
    uint32_t testIndex = 0;
#ifdef TEST_XMODEM_HEADER
    const char* testBlock = HeaderBlock;
#else
    const char* testBlock = DataBlock0;
#endif
    _readFunc = [&testBlock, &testIndex](uint8_t& c) { c = testBlock[testIndex++]; };
    _writeFunc = [](uint8_t c) { };
    _rxReadyFunc = []() -> bool { return true; };
#endif
    // block numbers start with 1

    // 132 byte packet
    // starts with SOH
    // block number byte
    // 255-block number
    // 128 bytes of data
    // checksum byte (whole packet)
    // a single EOT instead of SOH when done, send an ACK on it too

    uint32_t block = 1;
    uint32_t state = 0;
    uint32_t crc = 0;
    uint32_t startTime = _systemTime();
    
    static constexpr uint32_t MaxFilenameLength = 255;
    char filename[MaxFilenameLength + 1];
    int32_t size = -1;
    bool firstBlock = true;
    bool ymodem = false;
    
    uint32_t packetSize = 128;
    
    uint8_t xstring[256];

    while(1)
    {
        int64_t curTime = _systemTime();
        if ((curTime - startTime) >= 1000)
        {
            _writeFunc(NAK);
            startTime += 1000;
        }
        
        if (!_rxReadyFunc()) {
            continue;
        }
        
        _readFunc(xstring[state]);

#ifdef CAPTURE_DATA
        if (_bufferIndex < 512) {
            _buffer[_bufferIndex++] = xstring[state];
        }
#endif
        
        startTime = _systemTime();
        
        if (state == 0) {
            if (xstring[state] == EOT || xstring[state] == 0x1b) {
                _writeFunc(NAK);
                _writeFunc(ACK);
                
                // If this is ymodem, it's going to want to send more stuff.
                // It's a batch format and they're going to want to send 
                // and end file packet. CANcel all that.
                //
                // Maybe it's just Serial.app, but after you cancel, it's 
                // still looking for more characters. Send it a bunch until
                // it's satisfied.
                if (ymodem) {
                    _writeFunc(CAN);
                    _writeFunc(CAN);
                    for (int i = 0; i < 650; ++i) {
                        _writeFunc('.');
                    }
                    _writeFunc('\r');
                    _writeFunc('\n');
                    _writeFunc('\r');
                    _writeFunc('\n');
                }

#ifdef CAPTURE_DATA
                showXModemData();
#endif

                return xstring[state] == EOT;
            }
        }
        
        switch (state) {
        case 0:
            if (xstring[state] == SOH) {
                crc = xstring[state];
                state++;
            } else {
                _writeFunc(NAK);
            }
            break;
        case 1:
            // Block 0 holds the filename and size
            if (xstring[state] == 0 && firstBlock) {
                ymodem = true;
            }
            if (xstring[state] == block || (xstring[state] == 0 && firstBlock)) {
                crc += xstring[state];
                state++;
            } else {
                state = 0;
                _writeFunc(NAK);
            }
            break;
        case 2:
            if (xstring[state] == (0xFF - xstring[state - 1])) {
                crc += xstring[state];
                state++;
            } else {
                _writeFunc(NAK);
                state = 0;
            }
            break;
        case 131:
            crc &= 0xFF;
            if (xstring[state] == crc) {
                if (xstring[1] == 0 && firstBlock) {
                    // Header block
                    firstBlock = false;
                    
                    uint32_t index = 0;
                    for ( ; index < packetSize; ++index) {
                        filename[index] = xstring[index + 3];
                        if (filename[index] == '\0') {
                            break;
                        }                        
                    }
                    
                    if (index == packetSize) {
                        // Bad header
                        _writeFunc(NAK);
                        state = 0;
                        break;
                    }
                    
                    // Get the size string
                    uint32_t offset = index + 1;
                    size = 0;
                    for (index = 0; index + offset < packetSize; ++index) {
                        char c = xstring[index + offset + 3];
                        if (c == ' ') {
                            break;
                        }

                        if (c < '0' || c > '9') {
                            // Bad header
                            _writeFunc(NAK);
                            state = 0;
                            break;
                        }
                        
                        size *= 10;
                        size += c - '0';
                    }
                    
                    if (index + offset == packetSize) {
                        // Bad header
                        _writeFunc(NAK);
                        state = 0;
                        break;
                    }
                } else {
                    uint32_t sizeToWrite = packetSize;
                    if (size >= 0) {
                        if (static_cast<uint32_t>(size) < packetSize) {
                            sizeToWrite = size;
                        }
                        size -= sizeToWrite;
                    }
                    
                    for (uint32_t i = 0; i < sizeToWrite; i++) {
                        if (!func(xstring[i + 3])) {
                            _writeFunc(ACK);
                            return false;
                        }
                    }
                    
                    block = (block + 1) & 0xFF;
                }
                _writeFunc(ACK);
#ifdef TEST_XMODEM
                if (testBlock == HeaderBlock) {
                    testBlock = DataBlock0;
                    testIndex = 0;
                } else if (testBlock == DataBlock0) {
                    testBlock = DataBlock1;
                    testIndex = 0;
                } else {
                    return true;
                }
#endif
            } else {
                _writeFunc(NAK);
            }
            state = 0;
            break;
        default:
            crc += xstring[state];
            state++;
            break;
        }
    }
}
