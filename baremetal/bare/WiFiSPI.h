/*
  WiFiSPI.h - Library for Arduino SPI connection to ESP8266
  Copyright (c) 2017 Jiri Bilek. All rights reserved.

 Circuit:
   1. On ESP8266 must be running (flashed) WiFiSPIESP application.
    
   2. Connect the Arduino to the following pins on the esp8266:

            ESP8266         |
    GPIO    NodeMCU   Name  |   Uno
   ===================================
     15       D8       SS   |   D10
     13       D7      MOSI  |   D11
     12       D6      MISO  |   D12
     14       D5      SCK   |   D13

    Note: If the ESP is booting at a moment when the SPI Master (i.e. Arduino) has the Select line HIGH (deselected)
    the ESP8266 WILL FAIL to boot!

  -----
  
  Based on WiFi.h - Library for Arduino Wifi shield.
  Copyright (c) 2011-2014 Arduino LLC.  All right reserved.

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

#include <cstdint>
#include "bare/IPAddress.h"
#include "WiFiSPIDriver.h"

namespace bare {

    static constexpr uint8_t WL_FW_VER_LENGTH = 6; // (format a.b.c)
    static constexpr uint8_t WL_PROTOCOL_VER_LENGTH = 6; // (format a.b.c)
    static constexpr uint8_t WL_SSID_MAX_LENGTH = 32;
    static constexpr uint8_t WL_WPA_KEY_MAX_LENGTH = 63;
    static constexpr uint8_t WL_WEP_KEY_MAX_LENGTH = 13;
    static constexpr uint8_t WL_MAC_ADDR_LENGTH = 6;
    static constexpr uint8_t WL_IPV4_LENGTH = 4;
    static constexpr uint8_t WL_NETWORKS_LIST_MAXNUM = 10;
    static constexpr uint8_t MAX_SOCK_NUM = 4;
    static constexpr uint8_t SOCK_NOT_AVAIL = 255;
    static constexpr uint8_t NA_STATE = -1;
    static constexpr uint8_t WL_MAX_ATTEMPT_CONNECTION = 20;
    static constexpr uint8_t SS = 8;

    class WiFiSPI
    {
    public:
        enum class Status {
            NoShield = 255,
            Idle            = 0,
            NoSSIDAvail     = 1,
            ScanCompleted   = 2,
            Connected       = 3,
            ConnectFailed   = 4,
            ConnectionLost  = 5,
            Disconnected    = 6,
            Failure         = 128,
            Success         = 129,
            Scanning        = 130,
        };
        
        WiFiSPI(SPIMaster* spi) : _driver(spi) { }
        
        /*
         * Initialization of the library.
         *
         * param pin: SS pin, default value get default pin
         * param max_speed: maximum speed of SPI interface
         */
        void init(int8_t pin = -1, uint32_t max_speed = 0);

        uint8_t getSocket();

        String firmwareVersion() { return getStringCmd(WiFiSPIDriver::Command::GET_FW_VERSION, WL_FW_VER_LENGTH); }
        String protocolVersion() { return getStringCmd(WiFiSPIDriver::Command::GET_PROTOCOL_VERSION, WL_PROTOCOL_VER_LENGTH); }
        String SSID() { return getStringCmd(WiFiSPIDriver::Command::GET_CURR_SSID, WL_SSID_MAX_LENGTH); }

        Status status() { return getStatusCmd(WiFiSPIDriver::Command::GET_CONN_STATUS); }
        Status connect(const char* ssid) { return sendParamsCmd(WiFiSPIDriver::Command::SET_NET, ssid); }
        Status connect(const char* ssid, const char* pwd) { return sendParamsCmd(WiFiSPIDriver::Command::SET_PASSPHRASE, ssid, pwd); }
        Status disconnect() { return getStatusCmd(WiFiSPIDriver::Command::DISCONNECT); }

        // Change Ip configuration settings disabling the dhcp client
        bool setConfig(IPAddress local_ip);
        bool setConfig(IPAddress local_ip, IPAddress dns_server);
        bool setConfig(IPAddress local_ip, IPAddress dns_server, IPAddress gateway);
        bool config(IPAddress local_ip, IPAddress dns_server, IPAddress gateway, IPAddress subnet);

        bool setDNS(IPAddress dns_server1);
        bool setDNS(IPAddress dns_server1, IPAddress dns_server2);

        uint8_t* macAddress(uint8_t* mac);
        IPAddress localIP();
        IPAddress subnetMask();
        IPAddress gatewayIP();
        uint8_t* BSSID();
        
        // Return the signal strength
        int32_t RSSI();

        //Start scan WiFi networks available
        //return: Number of discovered networks
        Status startNetworkScan();
        Status checkNetworkScan(uint8_t& numNetworks);

        // Return the i-th discovered network from scanNetworks
        Status scannedNetworkItem(uint8_t i, String& ssid, int32_t& rssi, uint8_t& encryptionType);
     
        Status hostByName(const char* aHostname, IPAddress& aResult);

        void softReset();
        
        static const char* statusDetail(Status);

    private:
        String getStringCmd(WiFiSPIDriver::Command, uint8_t length);
        Status getStatusCmd(WiFiSPIDriver::Command);
        uint8_t getUInt8Cmd(WiFiSPIDriver::Command);
        
        // Template function to send command with variable number of params
        // call sendParamsCmd(), as long as there is a driver.sendParam()
        // method with the appropriate type, you can use that type in the
        // parameter list
        template<typename ... Args>
        Status sendParamsCmd(WiFiSPIDriver::Command cmd, Args... args)
        {
            _driver.sendCmd(cmd);
            sendParams(args...);
            _driver.endCmd();
            return waitForStatus(cmd);
        }
        
        template<typename T>
        void sendParams(T first)
        {
            _driver.sendParam(first);
        }

        template<typename T, typename... Args>
        void sendParams(T first, Args... args)
        {
            sendParams(first);
            sendParams(args...);
        }

        // Template function to send command which expects a variable number of results
        template<typename ... Args>
        Status getParamsCmd(WiFiSPIDriver::Command cmd, Args&... args)
        {
            _driver.sendCmd(cmd);
            
            uint8_t paramCount = 0;
            countParams(paramCount, args...);
            if (_driver.waitResponseStart(cmd, paramCount)) {
                waitParams(args...);
                if (_driver.waitResponseEnd()) {
                    return Status::Success;
                }
            }
            return Status::Failure;
        }
        
        template<typename T, typename ... Args>
        Status getParamsWithSentParamCmd(WiFiSPIDriver::Command cmd, T param, Args&... args)
        {
            _driver.sendCmd(cmd, 1);
            _driver.sendParam(param);
            _driver.endCmd();
            
            uint8_t paramCount = 0;
            countParams(paramCount, args...);
            if (_driver.waitResponseStart(cmd, paramCount)) {
                waitParams(args...);
                if (_driver.waitResponseEnd()) {
                    return Status::Success;
                }
            }
            return Status::Failure;
        }
        
        template<typename T>
        void countParams(uint8_t& paramCount, T first)
        {
            ++paramCount;
        }
        
        template<typename T, typename... Args>
        void countParams(uint8_t& paramCount, T first, Args... args)
        {
            ++paramCount;
            countParams(paramCount, args...);
        }

        template<typename T>
        void waitParams(T& first)
        {
            _driver.waitResponseParam(first);
        }
        
        template<typename T, typename... Args>
        void waitParams(T& first, Args&... args)
        {
            _driver.waitResponseParam(first);
            waitParams(args...);
        }

        Status waitForUInt8(WiFiSPIDriver::Command, uint8_t&);
        Status waitForStatus(WiFiSPIDriver::Command cmd)
        {
            uint8_t value;
            if (waitForUInt8(cmd, value) == Status::Failure) {
                return Status::Failure;
            }
            return static_cast<Status>(value);
        }

        int16_t  _state[MAX_SOCK_NUM];
        uint16_t _server_port[MAX_SOCK_NUM];
        WiFiSPIDriver _driver;
    };

}
