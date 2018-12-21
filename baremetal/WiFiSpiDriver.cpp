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

#include "bare/WiFiSpiDriver.h"

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
    
    _spi->startTransfer();
    _spi->transferByte(static_cast<uint8_t>(Command::START), ResponseTimeout);
    _spi->transferByte(static_cast<uint8_t>(cmd), ResponseTimeout);
    _spi->transferByte(numParam, ResponseTimeout);
    if (numParam == 0) {
        endCmd();
    }
    DEBUG_LOG("WiFiSpi:sendCmd finished\n");
}

void WiFiSpiDriver::endCmd()
{
    _spi->transferByte(static_cast<uint8_t>(Command::END), ResponseTimeout);
    _spi->endTransfer();
}

/*
    Sends a parameter.
    param ... parameter value
    param_len ... length of the parameter
 */
void WiFiSpiDriver::sendParam(const uint8_t* param, uint8_t param_len)
{
    DEBUG_LOG("WiFiSpi:sendParam(length=%d)\n", param_len);

    _spi->transferByte(param_len, ResponseTimeout);

    for (int i = 0; i < param_len; ++i) {
        _spi->transferByte(param[i], ResponseTimeout);
    }
}

/*
    Sends a 8 bit integer parameter. Sends high byte first.
    param ... parameter value
 */
void WiFiSpiDriver::sendParam(uint8_t param)
{
    DEBUG_LOG("WiFiSpi:sendParam(param=%#02x)\n", param);

    _spi->transferByte(1, ResponseTimeout);
    _spi->transferByte(param, ResponseTimeout);
}


/*
    Sends a buffer as a parameter.
    Parameter length is 16 bit.
 */
void WiFiSpiDriver::sendBuffer(const uint8_t* param, uint16_t param_len)
{
    DEBUG_LOG("WiFiSpi:sendBuffer(length=%d)\n", param_len);

    _spi->transferByte(param_len & 0xff, ResponseTimeout);
    _spi->transferByte(param_len >> 8, ResponseTimeout);

    // Send param data
    for (uint16_t i=0;  i<param_len;  ++i) {
        _spi->transferByte(param[i], ResponseTimeout);
    }
}

/*
    Gets a response from the ESP
    cmd ... command id
    numParam ... number of parameters - currently supported 0 or 1
    param  ... pointer to space for the first parameter
    param_len ... max length of the first parameter, returns actual length
 */
bool WiFiSpiDriver::waitResponse(Command cmd, uint8_t numParam, uint8_t* param, uint8_t& param_len)
{
    DEBUG_LOG("WiFiSpi:waitResponse(cmd=%#02x, num=%d, len=%d)\n", cmd, numParam, param_len);

    bool result = false;
    _spi->startTransfer();

    if (readAndCheckByte(Command::START, "Start") &&
            readAndCheckByte(setReply(cmd), "Cmd") &&
            readAndCheckByte(numParam, "Param")) {    
        if (numParam == 1)
        {
            int32_t len = _spi->transferByte(0, ResponseTimeout);
            if (len >= 0) {
                for (uint8_t ii=0; ii<len; ++ii)
                {
                    if (ii < param_len) {
                        param[ii] = _spi->transferByte(0, ResponseTimeout);
                    }
                }

                if (len < param_len) {
                    param_len = len;
                }
            }
        }
        else if (numParam == 0) {    
            result = readAndCheckByte(Command::END, "End");
        }
    }
    _spi->endTransfer();
    return result;
}

/*
    Gets a response from the ESP
    cmd ... command id
    numParam ... number of parameters - currently supported 0 or 1
    param  ... pointer to space for the first parameter
    param_len ... max length of the first parameter (16 bit integer), returns actual length
 */
bool WiFiSpiDriver::waitResponse(Command cmd, uint8_t numParam, uint8_t* param, uint16_t& param_len)
{
    DEBUG_LOG("WiFiSpi:waitResponse[16](cmd=%#02x, num=%d, len=%d)\n", cmd, numParam, param_len);

    if (!readAndCheckByte(Command::START, "Start")) {
        return false;
    }
    
    if (!readAndCheckByte(setReply(cmd), "Cmd")) {
        return false;
    }
    
    if (!readAndCheckByte(numParam, "Param")) {
        return false;
    }

    if (numParam == 1)
    {
        int32_t v = _spi->transferByte(0, ResponseTimeout);
        if (v < 0) {
            return false;
        }
        
        uint16_t len = v << 8;
        
        v = _spi->transferByte(0, ResponseTimeout);
        if (v < 0) {
            return false;
        }
        
        len |= v;
        
        for (uint16_t ii=0; ii<len; ++ii) {
            if (ii < param_len) {
                v = _spi->transferByte(0, ResponseTimeout);
                if (v < 0) {
                    return false;
                }
                param[ii] = v;
            }
        }

        if (len < param_len) {
            param_len = len;
        }
    }
    else if (numParam != 0) {
        return false;
    }
  
    return readAndCheckByte(Command::END, "End");
}

/*
    Reads a response from the ESP. Decodes parameters and puts them into a return structure
 */
bool WiFiSpiDriver::waitResponse(Command cmd, uint8_t numParam, Param* params)
{
    DEBUG_LOG("WiFiSpi:waitResponse[Params](cmd=%#02x, num=%d)\n", cmd, numParam);

    if (!readAndCheckByte(Command::START, "Start")) {
        return false;
    }
    
    if (!readAndCheckByte(setReply(cmd), "Cmd")) {
        return false;
    }
    
    if (!readAndCheckByte(numParam, "Param")) {
        return false;
    }

    if (numParam > 0) {
        for (uint8_t i=0;  i < numParam;  ++i) {
            int32_t len = _spi->transferByte(0, ResponseTimeout);
            if (len < 0) {
                return false;
            }

            for (uint8_t ii = 0; ii < len; ++ii) {
                if (ii < params[i].length) {
                    int32_t v = _spi->transferByte(0, ResponseTimeout);
                    if (v < 0) {
                        return false;
                    }
                    params[i].value[ii] = v;
                }
            }

            if (len < params[i].length) {
                params[i].length = len;
            }
        }
    }
    
    return readAndCheckByte(Command::END, "End");
}

