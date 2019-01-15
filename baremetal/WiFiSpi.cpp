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

#include "bare/SPIMaster.h"
#include "bare/Timer.h"

//#define ENABLE_DEBUG_LOG
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
    
    SPIMaster::init(20000);
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
    DEBUG_LOG("WiFiSpi::getStatusCmd:(0x%02x)\n", static_cast<uint32_t>(cmd));
    _driver.sendCmd(cmd);
    DEBUG_LOG("WiFiSpi::getStatusCmd:command sent\n");
    
    return waitForStatus(cmd);
}

uint8_t WiFiSpi::getUInt8Cmd(WiFiSpiDriver::Command cmd)
{
    DEBUG_LOG("WiFiSpi::getUInt8Cmd:(0x%02x)\n", static_cast<uint32_t>(cmd));
    _driver.sendCmd(cmd);
    
    uint8_t value;
    waitForUInt8(cmd, value);
    return value;
}

WiFiSpi::Status WiFiSpi::waitForUInt8(WiFiSpiDriver::Command cmd, uint8_t& value)
{
    if (!_driver.waitResponse(cmd, value)) {
        return WiFiSpi::Status::Failure;
    }
    return WiFiSpi::Status::Success;
}

const char* WiFiSpi::statusDetail(Status status)
{
    switch (status) {
    case Status::NoShield       : return "no shield";
    case Status::Idle           : return "idle";
    case Status::NoSSIDAvail    : return "no SSID available";
    case Status::ScanCompleted  : return "scan completed";
    case Status::Connected      : return "connected";
    case Status::ConnectFailed  : return "connect failed";
    case Status::ConnectionLost : return "connection lost";
    case Status::Disconnected   : return "disconnected";
    case Status::Failure        : return "failure";
    case Status::Success        : return "success";
    default                     : return "*** unknown ***";
    }
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
 * Perform remote software reset of the ESP8266 module. 
 * The reset succeedes only if the SPI communication is not broken.
 * The function does not wait for the ESP8266.
 */
void WiFiSpi::softReset(void) {
//    WiFiSpiDrv::softReset();
}

static constexpr int8_t WiFiScanRunning = -1;
static constexpr int8_t WiFiScanFailed = -2;

WiFiSpi::Status WiFiSpi::startNetworkScan()
{
    int8_t num = getUInt8Cmd(WiFiSpiDriver::Command::START_SCAN_NETWORKS);
    return (num == WiFiScanFailed) ? Status::Failure : Status::Scanning;
}

WiFiSpi::Status WiFiSpi::checkNetworkScan(uint8_t& numNetworks)
{
    int8_t num = getUInt8Cmd(WiFiSpiDriver::Command::SCAN_NETWORKS);
    if (num == WiFiScanFailed) {
        return Status::Failure;
    }
    numNetworks = (num < 0) ? 0 : static_cast<uint8_t>(num);
    return (num == WiFiScanRunning) ? Status::Scanning : Status::ScanCompleted;
}

WiFiSpi::Status WiFiSpi::scannedNetworkItem(uint8_t i, String& ssid, uint8_t& encryptionType, int32_t& rssi)
{
    char ssidBuffer[WiFiSpiDriver::MaxSSIDSize];
    WiFiSpiDriver::Param params[] =
    {
        { WiFiSpiDriver::MaxSSIDSize, ssidBuffer },
        { sizeof(rssi), reinterpret_cast<char*>(&rssi) },
        { sizeof(encryptionType), reinterpret_cast<char*>(&encryptionType) }
    };
        
    _driver.sendCmd(WiFiSpiDriver::Command::GET_SCANNED_DATA, 1);
    _driver.sendParam(i);
    _driver.endCmd();
    Status status = _driver.waitResponse(WiFiSpiDriver::Command::GET_SCANNED_DATA, 3, params) ? Status::Success : Status::Failure;
    ssid = String(ssidBuffer, params[0].length);
    return status;
}

