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
#ifndef _NDEBUG
void WiFiSpiDriver::showCheckError(const char* err, uint8_t expected, uint8_t got)
{
    Serial::printf("%s exp:%#02x, got:%#02x\n", err, expected, got);
}
#endif

static inline uint8_t addReply(WiFiSpiDriver::Command cmd)
{
    return static_cast<uint8_t>(cmd) | REPLY_FLAG;
}

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
    _spi->waitForSlaveRxReady();
    
    // Send Spi START CMD
    _spi->write(static_cast<uint8_t>(Command::START));

    // Send Spi C + cmd
    _spi->write(addReply(cmd));

    // Send Spi numParam
    _spi->write(numParam);

    // If numParam == 0 send END CMD and flush
    if (numParam == 0)
        endCmd();
}

/*
   Ends a command and flushes the buffer
 */
void WiFiSpiDriver::endCmd()
{
    _spi->write(static_cast<uint8_t>(Command::END));
    //_spi->flush(MESSAGE_FINISHED);
}

/*
    Sends a parameter.
    param ... parameter value
    param_len ... length of the parameter
 */
void WiFiSpiDriver::sendParam(const uint8_t* param, uint8_t param_len)
{
    // Send paramLen
    _spi->write(param_len);

    // Send param data
    for (int i=0; i<param_len; ++i)
        _spi->write(param[i]);
}

/*
    Sends a 8 bit integer parameter. Sends high byte first.
    param ... parameter value
 */
void WiFiSpiDriver::sendParam(uint8_t param)
{
    // Send paramLen
    _spi->write(1);

    // Send param data
    _spi->write(param );
}


/*
    Sends a buffer as a parameter.
    Parameter length is 16 bit.
 */
void WiFiSpiDriver::sendBuffer(const uint8_t* param, uint16_t param_len)
{
    // Send paramLen
    _spi->write(param_len & 0xff);
    _spi->write(param_len >> 8);

    // Send param data
    for (uint16_t i=0;  i<param_len;  ++i)
        _spi->write(param[i]);
}

/*
    Gets a response from the ESP
    cmd ... command id
    numParam ... number of parameters - currently supported 0 or 1
    param  ... pointer to space for the first parameter
    param_len ... max length of the first parameter, returns actual length
 */
uint8_t WiFiSpiDriver::waitResponseCmd(Command cmd, uint8_t numParam, uint8_t* param, uint8_t& param_len)
{
    _spi->waitForSlaveTxReady();
    readAndCheckByte(Command::START, "Start");
    readAndCheckByte(addReply(cmd), "Cmd");
    readAndCheckByte(numParam, "Param");

    if (numParam == 1)
    {
        // Reads the length of the first parameter
        uint8_t len = _spi->read();

        // Reads the parameter, checks buffer overrun
        for (uint8_t ii=0; ii<len; ++ii)
        {
            if (ii < param_len)
                param[ii] = _spi->read();
        }

        // Sets the actual length of the parameter
        if (len < param_len)
            param_len = len;
    }
    else if (numParam != 0) {
        return 0;  // Bad number of parameters
    }
    
    readAndCheckByte(Command::END, "End");

    return 1;
}

/*
    Gets a response from the ESP
    cmd ... command id
    numParam ... number of parameters - currently supported 0 or 1
    param  ... pointer to space for the first parameter
    param_len ... max length of the first parameter (16 bit integer), returns actual length
 */
uint8_t WiFiSpiDriver::waitResponseCmd16(Command cmd, uint8_t numParam, uint8_t* param, uint16_t& param_len)
{
    _spi->waitForSlaveTxReady();
    readAndCheckByte(Command::START, "Start");
    readAndCheckByte(addReply(cmd), "Cmd");
    readAndCheckByte(numParam, "Param");

    if (numParam == 1)
    {
        // Reads the length of the first parameter
        uint16_t len = _spi->read() << 8;
        len |= _spi->read();
        // Reads the parameter, checks buffer overrun
        for (uint16_t ii=0; ii<len; ++ii)
        {
            if (ii < param_len)
                param[ii] = _spi->read();
        }

        // Sets the actual length of the parameter
        if (len < param_len)
            param_len = len;
    }
    else if (numParam != 0)
        return 0;  // Bad number of parameters
  
    readAndCheckByte(Command::END, "End");

    return 1;
}

/*
    Reads a response from the ESP. Decodes parameters and puts them into a return structure
 */
int8_t WiFiSpiDriver::waitResponseParams(Command cmd, uint8_t numParam, Param* params)
{
    _spi->waitForSlaveTxReady();
    readAndCheckByte(Command::START, "Start");
    readAndCheckByte(addReply(cmd), "Cmd");
    readAndCheckByte(numParam, "Param");

    if (numParam > 0)
    {
        for (uint8_t i=0;  i<numParam;  ++i)
        {
            // Reads the length of the first parameter
            uint8_t len = _spi->read();

            // Reads the parameter, checks buffer overrun
            for (uint8_t ii=0; ii<len; ++ii)
            {
                if (ii < params[i].length)
                    params[i].value[ii] = _spi->read();
            }

            // Sets the actual length of the parameter
            if (len < params[i].length)
                params[i].length = len;
        }
    }
    readAndCheckByte(Command::END, "End");

    return 1;
}

