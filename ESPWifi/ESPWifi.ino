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

// Ports
//
//      D5 - SS
//      D6 - MISO
//      D7 - MOSI
//      D8 - SCK
//

#include "bare.h"

#include "bare/GPIO.h"
#include "bare/Serial.h"
#include "bare/Timer.h"

#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

static constexpr uint32_t ActivityLED = BUILTIN_LED;

// Number of ms LED stays off in each mode
constexpr uint32_t BlinkSampleRate = 10;
const uint32_t ConnectingRate = 400;
const uint32_t ConfigRate = 100;
const uint32_t ConnectedRate = 1900;

bool connecting = true;

class LEDBlinker : public bare::TimerCallback
{
public:
    LEDBlinker(uint32_t led) : _led(led)
    {
        bare::GPIO::setFunction(_led, bare::GPIO::Function::Output);
    }

    void start(uint32_t rate)
    {
        bare::Timer::start(this, rate, true);
    }

    virtual void handleTimerEvent()
    {
        bare::GPIO::setPin(_led, !bare::GPIO::getPin(_led));
    }

private:
    uint32_t _led;
};

//gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager)
{
    Serial.println("Entered config mode");
    Serial.println(WiFi.softAPIP());
    //if you used auto generated SSID, print it
    Serial.println(myWiFiManager->getConfigPortalSSID());
    //entered config mode, make led toggle faster
    blinker.setRate(ConfigRate);
}

void setup()
{
    bare::Serial::init();

    bare::Serial::printf("\n\nPlacid Wifi v0.1\n\n");

    LEDBlinker blinker(ActivityLED);
    blinker.start(ConnectingRate);

    // WiFiManager
    // Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    // Reset settings - for testing
    //wifiManager.resetSettings();
    
    // Set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wifiManager.setAPCallback(configModeCallback);
    
    // fetches ssid and pass and tries to connect
    // if it does not connect it starts an access point with the specified name
    // here  "AutoConnectAP"
    // and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect()) {
        bare::Serial::printf("failed to connect and hit timeout\n");
        // reset and try again, or maybe put it to deep sleep
        ESP.reset();
        bare::Timer::usleep(1000000);
    }

    //if you get here you have connected to the WiFi
    bare::Serial::printf("Wifi connected, IP=%s\n", WiFi.localIP());
    blinker.start(ConnectedRate);
}

void loop()
{
}
