/*
  espspi_drv.h - Library for Arduino SPI connection with ESP8266
  
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

#pragma once

#include "bare/SPIMaster.h"
#include <cstdint>

//#define ENABLE_DEBUG_LOG
#include "bare/Log.h"

namespace bare {

    class WiFiSpiDriver
    {
      public:
        static constexpr uint32_t ResponseTimeout = 3000000;
        
        enum class Command {
            WRITESTATUS          = 0x01,
            WRITEDATA            = 0x02,
            READDATA             = 0x03,
            READSTATUS           = 0x04,
            
            SET_NET              = 0x10,
            SET_PASSPHRASE       = 0x11,
            SET_KEY              = 0x12,
            SET_IP_CONFIG        = 0x14,

            GET_CONN_STATUS      = 0x20,
            GET_IPADDR           = 0x21,
            GET_MACADDR          = 0x22,
            GET_CURR_SSID        = 0x23,
            GET_CURR_BSSID       = 0x24,
            GET_CURR_RSSI        = 0x25,
            GET_CURR_ENCT        = 0x26,
            SCAN_NETWORKS        = 0x27,
            START_SERVER_TCP     = 0x28,
            GET_STATE_TCP        = 0x29,
            DATA_SENT_TCP        = 0x2A,
            AVAIL_DATA_TCP       = 0x2B,
            GET_DATA_TCP         = 0x2C,
            START_CLIENT_TCP     = 0x2D,
            STOP_CLIENT_TCP      = 0x2E,
            GET_CLIENT_STATE_TCP = 0x2F,
            DISCONNECT           = 0x30,
            GET_IDX_SSID         = 0x31,
            GET_IDX_RSSI         = 0x32,
            GET_IDX_ENCT         = 0x33,
            REQ_HOST_BY_NAME     = 0x34,
            GET_HOST_BY_NAME     = 0x35,
            START_SCAN_NETWORKS  = 0x36,
            GET_FW_VERSION       = 0x37,
            GET_TEST             = 0x38,
            SEND_DATA_UDP        = 0x39,
            GET_REMOTE_DATA      = 0x3A,

            // Not present in original protocol, added for ESP8266
            STOP_SERVER_TCP      = 0x3B,
            GET_SCANNED_DATA     = 0x3C,
            BEGIN_UDP_PACKET     = 0x3D,
            UDP_PARSE_PACKET     = 0x3E,
            SOFTWARE_RESET       = 0x3F,

            GET_PROTOCOL_VERSION = 0x50,

            // All command with DATA_FLAG 0x40 send a 16bit Len
            SEND_DATA_TCP        = 0x44,
            GET_DATABUF_TCP      = 0x45,
            INSERT_DATABUF       = 0x46,
            
            // Message indicators
            MESSAGE_CONTINUES    = 0xdc,
            MESSAGE_FINISHED     = 0xdf,

            // Start/End
            START                = 0xe0,
            END                  = 0xee,
        };

        struct Param
        {
            uint8_t length;
            char* value;
        };
        
        WiFiSpiDriver(SPIMaster* spi) : _spi(spi) { }

        void sendCmd(Command cmd, uint8_t numParam = 0);
        void endCmd();

        void sendParam(const uint8_t* param, uint8_t param_len);
        void sendParam(uint8_t param);

        void sendBuffer(const uint8_t* param, uint16_t param_len);

        bool waitResponse(Command cmd)
        {
            uint16_t len = 0;
            return waitResponse(cmd, 0, nullptr, len, false);
        }

        bool waitResponse(Command cmd, uint8_t& param)
        {
            uint16_t len = sizeof(uint8_t);
            return waitResponse(cmd, 1, &param, len, false);
        }

        bool waitResponse(Command cmd, uint16_t& param)
        {
            uint16_t len = sizeof(uint16_t);
            return waitResponse(cmd, 1, reinterpret_cast<uint8_t*>(&param), len, false);
        }

        bool waitResponse(Command cmd, uint32_t& param)
        {
            uint16_t len = sizeof(uint32_t);
            return waitResponse(cmd, 1, reinterpret_cast<uint8_t*>(&param), len, false);
        }

        bool waitResponse(Command cmd, uint8_t* param, uint8_t& param_len)
        {
            uint16_t len = param_len;
            bool result = waitResponse(cmd, 1, param, len, false);
            param_len = static_cast<uint8_t>(len);
            return result;
        }
            
        bool waitResponse(Command cmd, uint8_t numParam, Param* params);
        
    private:
        bool waitResponse(Command cmd, uint8_t numParam, uint8_t* param, uint16_t& param_len, bool paramLength16 = true);

        void showCheckError(const char* err, uint8_t expected, uint8_t got);
        
        bool readAndCheckByte(uint8_t expected, const char* err)
        {
            uint32_t value = read();
            bool success = true;
            if (value == SPIMaster::ErrorByte) {
                ERROR_LOG("Timeout on %s cmd:expected %#02x\n", err, expected);
                success = false;
            } else if (static_cast<uint8_t>(value) != expected && !_spi->simulatedData()) {
                ERROR_LOG("Mismatch on %s:expexted %#02x, got %#02x\n", err, expected, value);
                success = false;
            }
            return success;
        }

        bool readAndCheckByte(Command expected, const char* err) { return readAndCheckByte(static_cast<uint8_t>(expected), err); }
        
        void write(Command cmd) { write(static_cast<uint8_t>(cmd)); }
        
        void write(uint8_t c);
        uint8_t read();
        
        void fillBuffer();
        void writeBuffer();
        
        enum class MessageIndicator { Finished, Continues };
        void flush(MessageIndicator);
        
        static constexpr int64_t ReadyStatusTimeout = 3000000; // us
        static constexpr int64_t ReadTimeout = 1000000; // us
        
        // ReadyStatus is established by the master. Returned status has RxStatus in bits 31:28, TxStatus in bits 27:24
        // TxStatus: 0 - NoData, 1 - Ready, 2 - PreparingData
        // RxStatus: 0 - Busy, 1 - Ready
        enum class RxStatus { Busy = 0, Ready = 1, Invalid };
        enum class TxStatus { NoData = 0, Ready = 1, PreparingData = 2, Invalid };
        uint32_t readStatus();
        RxStatus waitForRxReady();
        TxStatus waitForTxReady();
        
        SPIMaster* _spi;
        
        static constexpr uint32_t BufferSizeMax = 32;

        uint8_t _buffer[BufferSizeMax];
        uint8_t _bufferSize = 0;
        uint8_t _bufferIndex = 0;
    };

}
