/*
  WiFiSPI.cpp - Library for Arduino SPI connection to ESP8266
  Copyright (c) 2017 Jiri Bilek. All rights reserved.

  -----
  
  Based on WiFi.cpp - Library for Arduino Wifi shield.
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

#include "bare/WiFiSpi.h"

#include "bare/SPI.h"

#define ENABLE_DEBUG_LOG
#include "bare/Log.h"

using namespace bare;

void WiFiSpi::init(int8_t pin, uint32_t max_speed)
{
    // Initialize the connection arrays
    for (uint8_t i = 0; i < MAX_SOCK_NUM; ++i) {
        _state[i] = NA_STATE;
        _server_port[i] = 0;
    }

    if (pin < 0) {
        pin = SS;
    }
    
    SPI::init(SPI::EnablePolarity::ActiveLow, SPI::ClockEdge::Falling, SPI::ClockPolarity::ActiveLow);
    
//    if (max_speed != 0)
//        spi_obj->beginTransaction(SPISettings(max_speed, MSBFIRST, SPI_MODE0));
}

uint8_t WiFiSpi::getSocket()
{
    for (uint8_t i = 0; i < MAX_SOCK_NUM; ++i)
    {
        if (WiFiSpi::_state[i] == NA_STATE)  // _state is for both server and client
             return i;
    }
    return SOCK_NOT_AVAIL;
}

String WiFiSpi::getStringCmd(WiFiSpiDriver::Command cmd, uint8_t length)
{
    _driver.sendCmd(cmd);

    char* buf = new char[length + 1];
    if (!_driver.waitResponse(cmd, reinterpret_cast<uint8_t*>(buf), length)) {
        return "***ERROR***";
    }
    buf[length] = '\0';
    String s(buf);
    delete [ ] buf;
    return s;
}

WiFiSpi::Status WiFiSpi::getStatusCmd(WiFiSpiDriver::Command cmd)
{
    _driver.sendCmd(cmd);

    uint8_t value;
    if (!_driver.waitResponse(cmd, value)) {
        return WiFiSpi::Status::Failure;
    }
    return static_cast<Status>(value);
}

/*
 * 
 */
WiFiSpi::Status WiFiSpi::begin(const char* ssid)
{
	Status status = Status::Idle;
//    uint8_t attempts = WL_MAX_ATTEMPT_CONNECTION;
//
//    if (WiFiSpiDrv::wifiSetNetwork(ssid, strlen(ssid)) != WL_FAILURE)
//    {
//        // TODO: Improve timing
//        do
//        {
//            delay(WL_DELAY_START_CONNECTION);
//            status = WiFiSpiDrv::getConnectionStatus();
//        }
//        while (((status == WL_IDLE_STATUS) || (status == WL_SCAN_COMPLETED) || (status == WL_DISCONNECTED)) && (--attempts > 0));
//    } else
//        status = WL_CONNECT_FAILED;
//   
    return status;
}

/*
 * 
 */
WiFiSpi::Status WiFiSpi::begin(const char* ssid, const char *passphrase)
{
    Status status = Status::Idle;
//    uint8_t attempts = WL_MAX_ATTEMPT_CONNECTION;
//
//    // SSID and passphrase for WPA connection
//    if (WiFiSpiDrv::wifiSetPassphrase(ssid, strlen(ssid), passphrase, strlen(passphrase)) != WL_FAILURE)
//    {
//      // TODO: Improve timing
//        do
//        {
//            delay(WL_DELAY_START_CONNECTION);
//            status = WiFiSpiDrv::getConnectionStatus();
//        }
//        while (((status == WL_IDLE_STATUS) || (status == WL_SCAN_COMPLETED) || (status == WL_DISCONNECTED)) && (--attempts > 0));
//    } else
//        status = WL_CONNECT_FAILED;
//    
    return status;
}

/*
 * 
 */
bool WiFiSpi::config(IPAddr local_ip)
{
	return false; // WiFiSpiDrv::config((uint32_t)local_ip, 0, 0, 0, 0);
}

/*
 * 
 */
bool WiFiSpi::config(IPAddr local_ip, IPAddr dns_server)
{
	return false; // WiFiSpiDrv::config((uint32_t)local_ip, 0, 0, (uint32_t)dns_server, 0);
}

/*
 * 
 */
bool WiFiSpi::config(IPAddr local_ip, IPAddr dns_server, IPAddr gateway)
{
	return false; // WiFiSpiDrv::config((uint32_t)local_ip, (uint32_t)gateway, 0, (uint32_t)dns_server, 0);
}

/*
 * 
 */
bool WiFiSpi::config(IPAddr local_ip, IPAddr dns_server, IPAddr gateway, IPAddr subnet)
{
	return false; // WiFiSpiDrv::config((uint32_t)local_ip, (uint32_t)gateway, (uint32_t)subnet, (uint32_t)dns_server, 0);
}

/*
 * 
 */
bool WiFiSpi::setDNS(IPAddr dns_server1)
{
    return false; // WiFiSpiDrv::config(0, 0, 0, (uint32_t)dns_server1, 0);
}

/*
 * 
 */
bool WiFiSpi::setDNS(IPAddr dns_server1, IPAddr dns_server2)
{
    return false; // WiFiSpiDrv::config(0, 0, 0, (uint32_t)dns_server1, (uint32_t)dns_server2);
}

/*
 * 
 */
uint8_t* WiFiSpi::macAddress(uint8_t* mac)
{
//    uint8_t* _mac = WiFiSpiDrv::getMacAddress();
//    memcpy(mac, _mac, WL_MAC_ADDR_LENGTH);
//    return mac;
    return nullptr;
}

/*
 * 
 */
IPAddr WiFiSpi::localIP()
{
//    IPAddr ret;
//    WiFiSpiDrv::getIpAddress(ret);
//    return ret;
    return IPAddr();
}

/*
 * 
 */
IPAddr WiFiSpi::subnetMask()
{
//    IPAddr ret;
//    WiFiSpiDrv::getSubnetMask(ret);
//    return ret;
    return IPAddr();
}

/*
 * 
 */
IPAddr WiFiSpi::gatewayIP()
{
//    IPAddr ret;
//    WiFiSpiDrv::getGatewayIP(ret);
//    return ret;
    return IPAddr();
}

/*
 * 
 */
uint8_t* WiFiSpi::BSSID()
{
	return nullptr; // WiFiSpiDrv::getCurrentBSSID();
}

/*
 * 
 */
int32_t WiFiSpi::RSSI()
{
    return -1; // WiFiSpiDrv::getCurrentRSSI();
}

/*
 * 
 */
int8_t WiFiSpi::scanNetworks()
{
//    #define WIFI_SCAN_RUNNING   (-1)
//    #define WIFI_SCAN_FAILED    (-2)
//
//    uint8_t attempts = 10;
//    int8_t numOfNetworks = 0;
//
//    sendCmd(Command::START_SCAN_NETWORKS);
//    
//    int8_t _data = -1;
//    uint8_t _dataLen = sizeof(_data);
//    if (!waitResponse(Command::START_SCAN_NETWORKS, 1, reinterpret_cast<uint8_t *>(&_data), &_dataLen)) {
//        return -1;
//    }
//
//    if (_data == WIFI_SCAN_FAILED) {
//        return -1;
//    }
//    
//    do {
//        Timer::usleep(2000000);
//        numOfNetworks = WiFiSpiDrv::getScanNetworks();
//
//        if (numOfNetworks == WIFI_SCAN_FAILED)
//            return WL_FAILURE;
//    }
//    while ((numOfNetworks == WIFI_SCAN_RUNNING) && (--attempts > 0));
//    
//    return numOfNetworks;
    return 0;
}

/*
 * 
 */
String WiFiSpi::SSID(uint8_t networkItem)
{
	return String(); // WiFiSpiDrv::getSSIDNetworks(networkItem);
}

/*
 * 
 */
int32_t WiFiSpi::RSSI(uint8_t networkItem)
{
	return -1; // WiFiSpiDrv::getRSSINetworks(networkItem);
}

/*
 * 
 */
uint8_t WiFiSpi::encryptionType(uint8_t networkItem)
{
  return 0; // WiFiSpiDrv::getEncTypeNetworks(networkItem);
}


/*
 * 
 */
int8_t WiFiSpi::hostByName(const char* aHostname, IPAddr& aResult)
{
	return -1; // WiFiSpiDrv::getHostByName(aHostname, aResult);
}

/*
 * Perform remote software reset of the ESP8266 module. 
 * The reset succeedes only if the SPI communication is not broken.
 * The function does not wait for the ESP8266.
 */
void WiFiSpi::softReset(void) {
//    WiFiSpiDrv::softReset();
}
