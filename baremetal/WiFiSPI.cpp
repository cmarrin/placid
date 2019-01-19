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

#include "bare/WiFiSPI.h"

#include "bare/SPIMaster.h"
#include "bare/Timer.h"

//#define ENABLE_DEBUG_LOG
#include "bare/Log.h"

using namespace bare;

void WiFiSPI::init(int8_t pin, uint32_t max_speed)
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

uint8_t WiFiSPI::getSocket()
{
    for (uint8_t i = 0; i < MAX_SOCK_NUM; ++i)
    {
        if (WiFiSPI::_state[i] == NA_STATE)  // _state is for both server and client
             return i;
    }
    return SOCK_NOT_AVAIL;
}

String WiFiSPI::getStringCmd(WiFiSPIDriver::Command cmd, uint8_t length)
{
    String s;
    getParamsCmd(cmd, s);
    return s;
}

WiFiSPI::Status WiFiSPI::getStatusCmd(WiFiSPIDriver::Command cmd)
{
    DEBUG_LOG("WiFiSPI::getStatusCmd:(0x%02x)\n", static_cast<uint32_t>(cmd));
    _driver.sendCmd(cmd);
    DEBUG_LOG("WiFiSPI::getStatusCmd:command sent\n");
    
    return waitForStatus(cmd);
}

uint8_t WiFiSPI::getUInt8Cmd(WiFiSPIDriver::Command cmd)
{
    DEBUG_LOG("WiFiSPI::getUInt8Cmd:(0x%02x)\n", static_cast<uint32_t>(cmd));
    _driver.sendCmd(cmd);
    
    uint8_t value;
    waitForUInt8(cmd, value);
    return value;
}

WiFiSPI::Status WiFiSPI::waitForUInt8(WiFiSPIDriver::Command cmd, uint8_t& value)
{
    if (!_driver.waitResponseStart(cmd, 1)) {
        return WiFiSPI::Status::Failure;
    }
    _driver.waitResponseParam(value);
    if (!_driver.waitResponseEnd()) {
        return WiFiSPI::Status::Failure;
    }
    return WiFiSPI::Status::Success;
}

const char* WiFiSPI::statusDetail(Status status)
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
bool WiFiSPI::setDNS(IPAddress dns_server1)
{
    return false; // WiFiSPIDrv::config(0, 0, 0, (uint32_t)dns_server1, 0);
}

/*
 * 
 */
bool WiFiSPI::setDNS(IPAddress dns_server1, IPAddress dns_server2)
{
    return false; // WiFiSPIDrv::config(0, 0, 0, (uint32_t)dns_server1, (uint32_t)dns_server2);
}

/*
 * 
 */
uint8_t* WiFiSPI::macAddress(uint8_t* mac)
{
//    uint8_t* _mac = WiFiSPIDrv::getMacAddress();
//    memcpy(mac, _mac, WL_MAC_ADDR_LENGTH);
//    return mac;
    return nullptr;
}

/*
 * 
 */
IPAddress WiFiSPI::localIP()
{
//    IPAddress ret;
//    WiFiSPIDrv::getIpAddress(ret);
//    return ret;
    return IPAddress();
}

/*
 * 
 */
IPAddress WiFiSPI::subnetMask()
{
//    IPAddress ret;
//    WiFiSPIDrv::getSubnetMask(ret);
//    return ret;
    return IPAddress();
}

/*
 * 
 */
IPAddress WiFiSPI::gatewayIP()
{
//    IPAddress ret;
//    WiFiSPIDrv::getGatewayIP(ret);
//    return ret;
    return IPAddress();
}

/*
 * 
 */
uint8_t* WiFiSPI::BSSID()
{
	return nullptr; // WiFiSPIDrv::getCurrentBSSID();
}

/*
 * 
 */
int32_t WiFiSPI::RSSI()
{
    return -1; // WiFiSPIDrv::getCurrentRSSI();
}

/*
 * Perform remote software reset of the ESP8266 module. 
 * The reset succeedes only if the SPI communication is not broken.
 * The function does not wait for the ESP8266.
 */
void WiFiSPI::softReset(void) {
//    WiFiSPIDrv::softReset();
}

static constexpr int8_t WiFiScanRunning = -1;
static constexpr int8_t WiFiScanFailed = -2;

WiFiSPI::Status WiFiSPI::startNetworkScan()
{
    int8_t num = getUInt8Cmd(WiFiSPIDriver::Command::START_SCAN_NETWORKS);
    return (num == WiFiScanFailed) ? Status::Failure : Status::Scanning;
}

WiFiSPI::Status WiFiSPI::checkNetworkScan(uint8_t& numNetworks)
{
    int8_t num = getUInt8Cmd(WiFiSPIDriver::Command::SCAN_NETWORKS);
    if (num == WiFiScanFailed) {
        return Status::Failure;
    }
    numNetworks = (num < 0) ? 0 : static_cast<uint8_t>(num);
    return (num == WiFiScanRunning) ? Status::Scanning : Status::ScanCompleted;
}

WiFiSPI::Status WiFiSPI::scannedNetworkItem(uint8_t i, String& ssid, int32_t& rssi, uint8_t& encryptionType)
{
    return getParamsWithSentParamCmd(WiFiSPIDriver::Command::GET_SCANNED_DATA, i, ssid, rssi, encryptionType);
}

