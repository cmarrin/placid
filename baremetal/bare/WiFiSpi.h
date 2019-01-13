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
#include "bare/IPAddr.h"
#include "WiFiSpiDriver.h"

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

    class WiFiSpi
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
        };
        
        WiFiSpi(SPIMaster* spi) : _driver(spi) { }
        
        /*
         * Initialization of the library.
         *
         * param pin: SS pin, default value get default pin
         * param max_speed: maximum speed of SPI interface
         */
        void init(int8_t pin = -1, uint32_t max_speed = 0);

        uint8_t getSocket();

        String firmwareVersion() { return getStringCmd(WiFiSpiDriver::Command::GET_FW_VERSION, WL_FW_VER_LENGTH); }
        String SSID() { return getStringCmd(WiFiSpiDriver::Command::GET_CURR_SSID, WL_SSID_MAX_LENGTH); }
        String protocolVersion() { return getStringCmd(WiFiSpiDriver::Command::GET_PROTOCOL_VERSION, WL_PROTOCOL_VER_LENGTH); }

        Status status() { return getStatusCmd(WiFiSpiDriver::Command::GET_CONN_STATUS); }
        Status disconnect() { return getStatusCmd(WiFiSpiDriver::Command::DISCONNECT); }

        /* Start Wifi connection for OPEN networks
         *
         * param ssid: Pointer to the SSID string.
         */
        Status begin(const char* ssid);

        /* Start Wifi connection with passphrase
         * the most secure supported mode will be automatically selected
         *
         * param ssid: Pointer to the SSID string.
         * param passphrase: Passphrase. Valid characters in a passphrase
         *        must be between ASCII 32-126 (decimal).
         */
        Status begin(const char* ssid, const char *passphrase);

        /* Change Ip configuration settings disabling the dhcp client
         *
         * param local_ip: 	Static ip configuration
         */
        bool config(IPAddr local_ip);

        /* Change Ip configuration settings disabling the dhcp client
         *
         * param local_ip: 	Static ip configuration
         * param dns_server:     IP configuration for DNS server 1
         */
        bool config(IPAddr local_ip, IPAddr dns_server);

        /* Change Ip configuration settings disabling the dhcp client
         *
         * param local_ip: 	Static ip configuration
         * param dns_server:     IP configuration for DNS server 1
         * param gateway : 	Static gateway configuration
         */
        bool config(IPAddr local_ip, IPAddr dns_server, IPAddr gateway);

        /* Change Ip configuration settings disabling the dhcp client
         *
         * param local_ip: 	Static ip configuration
         * param dns_server:     IP configuration for DNS server 1
         * param gateway: 	Static gateway configuration
         * param subnet:		Static Subnet mask
         */
        bool config(IPAddr local_ip, IPAddr dns_server, IPAddr gateway, IPAddr subnet);

        /* Change DNS Ip configuration
         *
         * param dns_server1: ip configuration for DNS server 1
         */
        bool setDNS(IPAddr dns_server1);

        /* Change DNS Ip configuration
         *
         * param dns_server1: ip configuration for DNS server 1
         * param dns_server2: ip configuration for DNS server 2
         *
         */
        bool setDNS(IPAddr dns_server1, IPAddr dns_server2);

        /*
         * Get the interface MAC address.
         *
         * return: pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
         */
        uint8_t* macAddress(uint8_t* mac);

        /*
         * Get the interface IP address.
         *
         * return: Ip address value
         */
        IPAddr localIP();

        /*
         * Get the interface subnet mask address.
         *
         * return: subnet mask address value
         */
        IPAddr subnetMask();

        /*
         * Get the gateway ip address.
         *
         * return: gateway ip address value
         */
       IPAddr gatewayIP();

        /*
         * Return the current BSSID associated with the network.
         * It is the MAC address of the Access Point
         *
         * return: pointer to uint8_t array with length WL_MAC_ADDR_LENGTH
         */
        uint8_t* BSSID();

        /*
         * Return the current RSSI /Received Signal Strength in dBm)
         * associated with the network
         *
         * return: signed value
         */
        int32_t RSSI();

        /*
         * Start scan WiFi networks available
         *
         * return: Number of discovered networks
         */
        int8_t scanNetworks();

        /*
         * Return the SSID discovered during the network scan.
         *
         * param networkItem: specify from which network item want to get the information
         *
         * return: ssid string of the specified item on the networks scanned list
         */
        String SSID(uint8_t networkItem);

        /*
         * Return the encryption type of the networks discovered during the scanNetworks
         *
         * param networkItem: specify from which network item want to get the information
         *
         * return: encryption type (enum wl_enc_type) of the specified item on the networks scanned list
         */
        uint8_t	encryptionType(uint8_t networkItem);

        /*
         * Return the RSSI of the networks discovered during the scanNetworks
         *
         * param networkItem: specify from which network item want to get the information
         *
         * return: signed value of RSSI of the specified item on the networks scanned list
         */
        int32_t RSSI(uint8_t networkItem);

        /*
         * Resolve the given hostname to an IP address.
         * param aHostname: Name to be resolved
         * param aResult: IPAddr structure to store the returned IP address
         * result: 1 if aIPAddrString was successfully converted to an IP address,
         *          else error code
         */
        int8_t hostByName(const char* aHostname, IPAddr& aResult);

        /*
         * Perform software reset of the ESP8266 module. 
         * The reset succeedes only if the SPI communication is not broken.
         * After the reset wait for the ESP8266 to came to life again. Typically, the ESP8266 boots within 100 ms,
         * but with the WifiManager installed on ESP it can be a couple of seconds.
         */
        void softReset(void);

    private:
        String getStringCmd(WiFiSpiDriver::Command, uint8_t length);
        Status getStatusCmd(WiFiSpiDriver::Command);

        int16_t  _state[MAX_SOCK_NUM];
        uint16_t _server_port[MAX_SOCK_NUM];
        WiFiSpiDriver _driver;
    };

}
