/*
  espspi_drv.cpp - Library for Arduino SPI connection with ESP8266
  
  Copyright (c) 2017 Jiri Bilek. All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include "WiFiSpiDriver.h"

#include "bare/Timer.h"

using namespace bare;

static constexpr uint8_t REPLY_FLAG = 1 << 7;


// Read and check one byte from the input
void WiFiSpiDriver::showCheckError(const char* err, uint8_t expected, uint8_t got)
{
    Serial::printf("%s exp:%#02x, got:%#02x\n", err, expected, got);
}

static inline uint8_t setReply(WiFiSpiDriver::Command cmd) { return static_cast<uint8_t>(cmd) | REPLY_FLAG; }

/* 
    Sends a command to ESP. If numParam == 0 ends the command otherwise keeps it open.
    
    Cmd Struct Message
   _______________________________________________________________________
  | START CMD | C/R  | CMD  | N.PARAM | PARAM LEN | PARAM  | .. | END CMD |
  |___________|______|______|_________|___________|________|____|_________|
  |   8 bit   | 1bit | 7bit |  8bit   |   8bit    | nbytes | .. |   8bit  |
  |___________|______|______|_________|___________|________|____|_________|
*/
void WiFiSpiDriver::sendCmd(Command cmd, uint8_t numParam)
{
    DEBUG_LOG("WiFiSpi:sendCmd(%#02x, %d)\n", static_cast<uint32_t>(cmd), numParam);
    
    write(Command::START);
    write(cmd);
    write(numParam);
    if (numParam == 0) {
        endCmd();
    }
}

void WiFiSpiDriver::endCmd()
{
    write(Command::END);
    flush(MessageIndicator::Finished);
    DEBUG_LOG("WiFiSpi:endCmd finished\n");
}

void WiFiSpiDriver::sendParam(const uint8_t* param, uint8_t param_len)
{
    DEBUG_LOG("WiFiSpi:sendParam(length=%d)\n", param_len);

    write(param_len);
    for (int i = 0; i < param_len; ++i) {
        write(param[i]);
    }
}

void WiFiSpiDriver::sendParam(uint8_t param)
{
    DEBUG_LOG("WiFiSpi:sendParam(param=%#02x)\n", param);

    write(1);
    write(param);
}

void WiFiSpiDriver::sendBuffer(const uint8_t* param, uint16_t param_len)
{
    DEBUG_LOG("WiFiSpi:sendBuffer(length=%d)\n", param_len);

    write(param_len & 0xff);
    write(param_len >> 8);

    for (uint16_t i = 0;  i < param_len;  ++i) {
        write(param[i]);
    }
}

/*
    Gets a response from the ESP
    cmd ... command id
    numParam ... number of parameters - currently supported 0 or 1
    param  ... pointer to space for the first parameter
    param_len ... max length of the first parameter (16 bit if paramLength16 is true, 8 bit otherwise), returns actual length
 */
bool WiFiSpiDriver::waitResponse(Command cmd, uint8_t numParam, uint8_t* param, uint16_t& param_len, bool paramLength16)
{
    DEBUG_LOG("WiFiSpi:waitResponse(cmd=%#02x, num=%d, len=%d)\n", cmd, numParam, param_len);

    bool result = false;
    waitForTxReady();

    if (readAndCheckByte(Command::START, "Start") &&
            readAndCheckByte(setReply(cmd), "Cmd") &&
            readAndCheckByte(numParam, "Param")) {    
        if (numParam == 1) {
            int16_t len = read();
            if (paramLength16) {
                len <<= 8;
                len |= read();
            }
            
            if (_spi->simulatedData()) {
                len = param_len;
            }
            
            for (uint16_t ii = 0; ii < len; ++ii) {
                if (ii < param_len) {
                    param[ii] = read();
                }
            }

            if (len < param_len) {
                param_len = len;
            }
        }
        else if (numParam != 0) {
            return false;
        }
        
        result = readAndCheckByte(Command::END, "End");
    }
    return result;
}

bool WiFiSpiDriver::waitResponse(Command cmd, uint8_t numParam, Param* params)
{
    DEBUG_LOG("WiFiSpi:waitResponse[Params](cmd=%#02x, num=%d)\n", cmd, numParam);

    bool result = false;
    waitForTxReady();

    if (readAndCheckByte(Command::START, "Start") &&
            readAndCheckByte(setReply(cmd), "Cmd") &&
            readAndCheckByte(numParam, "Param")) {    
        if (numParam > 0) {
            for (uint8_t i = 0; i < numParam; ++i) {
                uint8_t len = read();
                
                if (_spi->simulatedData()) {
                    len = params[i].length;
                }
                
                for (uint8_t ii = 0; ii < len; ++ii) {
                    if (ii < params[i].length) {
                        params[i].value[ii] = read();
                    }
                }

                if (len < params[i].length) {
                    params[i].length = len;
                }
            }
        }
        
        result = readAndCheckByte(Command::END, "End");
    }
    return result;
}

uint32_t WiFiSpiDriver::readStatus()
{
    _spi->startTransfer();
    _spi->transferByte(static_cast<uint8_t>(Command::READSTATUS));
    uint32_t status = _spi->transferByte(0);
    status |= static_cast<uint32_t>(_spi->transferByte(0)) << 8;
    status |= static_cast<uint32_t>(_spi->transferByte(0)) << 16;
    status |= static_cast<uint32_t>(_spi->transferByte(0)) << 24;
    _spi->endTransfer();
    return _spi->simulatedData() ? (((static_cast<uint32_t>(RxStatus::Ready) << 4) | static_cast<uint32_t>(TxStatus::Ready)) << 24) : status;
}

WiFiSpiDriver::RxStatus WiFiSpiDriver::waitForRxReady()
{
    int64_t endTime = Timer::systemTime() + ReadyStatusTimeout;
    uint32_t rawStatus;

    do {
        rawStatus = readStatus();
        RxStatus status = static_cast<RxStatus>(rawStatus >> 28);
        if (rawStatus) {
            DEBUG_LOG("********** waitForRxReady: Got a non-zero status: 0x%08x\n", rawStatus);
        }
        if (status == RxStatus::Ready) {
            return status;
        } else if (status != RxStatus::Busy) {
            ERROR_LOG("WiFiSpiDriver::waitForRxReady: Invalid status=0x%08x\n", rawStatus);
            return RxStatus::Invalid;
        }
    } while (Timer::systemTime() < endTime);

    ERROR_LOG("WiFiSpiDriver::waitForRxReady: timeout, status=0x%08x\n", rawStatus);
    
    return RxStatus::Busy;
}

WiFiSpiDriver::TxStatus WiFiSpiDriver::waitForTxReady()
{
    int64_t endTime = Timer::systemTime() + ReadyStatusTimeout;
    uint32_t rawStatus;
    TxStatus status;

    do {
        rawStatus = readStatus();
        status = static_cast<TxStatus>((rawStatus >> 24) & 0x0f);
        if (rawStatus) {
            DEBUG_LOG("********** waitForTxReady: Got a non-zero status: 0x%08x\n", rawStatus);
        }
        if (status == TxStatus::Ready) {
            return status;
        } else if (status != TxStatus::NoData && status != TxStatus::PreparingData) {
            ERROR_LOG("WiFiSpiDriver::waitForTxReady: Invalid status=0x%08x\n", rawStatus);
            return TxStatus::Invalid;
        }
    } while (Timer::systemTime() < endTime);

    ERROR_LOG("WiFiSpiDriver::waitForTxReady: timeout, status=0x%08x\n", rawStatus);
    
    return status;
}

uint8_t WiFiSpiDriver::read()
{
    // discard output data in the buffer
    _bufferSize = 0;
    Command cmd;
    
    if (_bufferIndex >= BufferSizeMax)
    {
        cmd = static_cast<Command>(_buffer[0]);
        if (cmd != Command::MESSAGE_CONTINUES) {
            return 0;
        }

        _bufferIndex = 0;
        
        waitForTxReady();
    }
    
    if (_bufferIndex == 0)
    {
        int64_t endTime = Timer::systemTime() + ReadTimeout;

        do {
            fillBuffer();
            cmd = _spi->simulatedData() ? Command::MESSAGE_FINISHED : static_cast<Command>(_buffer[0]);
            if (Timer::systemTime() >= endTime) {
                return 0;
            }
        }  while (cmd != Command::MESSAGE_FINISHED && cmd != Command::MESSAGE_CONTINUES);
        
        _bufferIndex = 1;
    }
    return _buffer[_bufferIndex++];
}
       
void WiFiSpiDriver::write(uint8_t c)
{
    // discard input data in the buffer
    _bufferIndex = 0;
    
    if (_bufferSize >= BufferSizeMax - 1) {
        flush(MessageIndicator::Continues);
    }
        
    _buffer[++_bufferSize] = c;
}

void WiFiSpiDriver::fillBuffer()
{
    _spi->startTransfer();
    
    _spi->transferByte(static_cast<uint8_t>(Command::READDATA));
    _spi->transferByte(0x00);
    for(uint8_t i = 0; i < BufferSizeMax; i++) {
        _buffer[i] = _spi->transferByte(0);
    }

    _spi->endTransfer();
}

void WiFiSpiDriver::writeBuffer()
{
    uint8_t i = 0;
    uint8_t len = _bufferSize + 1;
    
    _spi->startTransfer();
    
    _spi->transferByte(static_cast<uint8_t>(Command::WRITEDATA));
    _spi->transferByte(0x00);
    
    while(len-- && i < BufferSizeMax) {
        _spi->transferByte(_buffer[i++]);
    }
    
    while(i++ < BufferSizeMax) {
        _spi->transferByte(0);
    }

    _spi->endTransfer();
}

void WiFiSpiDriver::flush(MessageIndicator indicator)
{
    if (_bufferSize == 0) {
        return;
    }

    // Message state indicator
    _buffer[0] = static_cast<uint8_t>((indicator == MessageIndicator::Continues) ? Command::MESSAGE_CONTINUES : Command::MESSAGE_FINISHED);
    
    // Wait for slave ready
    if (waitForRxReady() == RxStatus::Ready) {
        writeBuffer();
    }
        
    _bufferSize = 0;
}
