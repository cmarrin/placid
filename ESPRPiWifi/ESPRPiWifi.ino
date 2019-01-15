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
//      D5 - SCLK
//      D6 - MISO
//      D7 - MOSI
//      D8 - SS
//

#include "bare.h"

#include "bare/GPIO.h"
#include "bare/Serial.h"
#include "bare/Shell.h"
#include "bare/SPISlave.h"
#include "bare/Timer.h"

#include <Ticker.h>
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

static constexpr uint32_t ActivityLED = BUILTIN_LED;

// Connections to Raspberry Pi:
//
//      Wemos                   RPi
//      =====                   ===
//      GPIO16 (D0)         ==> Reset pin
//      SCK (GPIO14, D5)    ==> SCK (BCM11, pin 23)
//      MOSI (GPIO13, D7)   ==> MOSI (BCM10, pin 19)
//      MISO (GPIO12, D6)   ==> MISO (BCM9, pin 21)
//      SS (GPIO15, D8)     ==> CE0 (BCM8, pin 24)

static constexpr uint32_t RPiResetPin = 16;
static constexpr uint32_t RPiRxPin = 4;
static constexpr uint32_t RPiTxPin = 5;

// Number of ms LED stays off in each mode
constexpr uint32_t BlinkSampleRate = 2;

// These rates are in ms
const uint32_t ConnectingRate = 800;
const uint32_t ConfigRate = 200;
const uint32_t ConnectedRate = 4000;
const uint32_t IdleRate = 4000;

uint32_t blinkRate = ConnectingRate;
uint32_t previousMillis = 0;
bool blinkState = false;

bool connecting = true;

class Blinker
{
public:
    Blinker()
    {
        bare::GPIO::setFunction(ActivityLED, bare::GPIO::Function::Output);
        _ticker.attach_ms(BlinkSampleRate, blink, this);
    }
    
    void setRate(uint32_t rate) { _rate = (rate + (BlinkSampleRate / 2)) / BlinkSampleRate; }
    
private:
    static void blink(Blinker* self)
    {
        if (self->_count == 0) {
            bare::GPIO::setPin(ActivityLED, false);
        } else if (self->_count == 1){
            bare::GPIO::setPin(ActivityLED, true);
        }
        if (++self->_count >= self->_rate) {
            self->_count = 0;
        }
    }
    
    Ticker _ticker;
    uint32_t _rate = IdleRate;
    uint32_t _count = 0;
};

Blinker blinker;

static void testSPI()
{
    bare::SPISlave spi;
    spi.init();
    bool finished = false;
    
    spi.setDataReceivedFunction([&spi, &finished](uint8_t* data, uint8_t size) {
        spi.setStatus(0);
        bare::String s(reinterpret_cast<const char*>(data), size);
        if (strcmp(reinterpret_cast<const char*>(data), "Are you there?") != 0) {
            bare::Serial::printf("*** Error:No match, got '%s'\n", s.c_str());
            finished = true;
        }
        
        spi.setData(reinterpret_cast<const uint8_t*>("I am here."), 11);
        bare::Serial::printf("*");
        
        // Let master know that I am both ready to receive more data and that I have sent some data
        spi.setStatus(0x03);
    });
    
    spi.setDataSentFunction([&spi]() {
        spi.setStatus(0x01);
    });
    
    spi.setStatusReceivedFunction([&finished](uint32_t status) {
        if (status & 0x04) {
            finished = true;
        }
    });
    
    spi.setStatusSentFunction([]() {
    });
    
    // Tell the master that the slave is ready to receive data
    spi.setStatus(0x01);
    
    // Wait 10 seconds for it to finish
    for (int i = 0; i < 100; ++i) {
        if (finished) {
            bare::Serial::printf("\nSPI test succeeded!!!\n\n");
            return;
        }
        
        bare::Timer::usleep(100000);
        yield();
    }
    bare::Serial::printf("\n******** SPI test timed out\n\n");
}

class MyShell : public bare::Shell
{
public:
    virtual const char* welcomeString() const override { return "\n\nPlacid ESP Shell v0.1"; }

    virtual const char* helpString() const override
    {
        return
                "    reset [pi/esp] : resest Raspberry Pi or ESP\n"
                "    test <id>      : run one of the built-in tests\n"
                "                         spi : run spi test\n"
        ;
    }

    virtual const char* promptString() const override { return "esp"; }

    virtual void shellSend(const char* data, uint32_t size, bool raw) override
    {
        // puts converts control characters to printable, so if we want
        // to send control we have to send raw
        if (raw) {
            while (size--) {
                bare::Serial::write(*data++);
            }
        } else {
            bare::Serial::puts(data, size);
        }   
    }

    virtual bool executeShellCommand(const std::vector<bare::String>& array) override
    {
        if (array[0] == "reset") {
            if (array.size() < 2) {
                showMessage(MessageType::Error, "use 'pi' or 'esp' to reset\n");
            } else if (array[1] == "pi") {
                showMessage(MessageType::Info, "Resetting Raspberry Pi\n");
                bare::GPIO::setPin(RPiResetPin, true);
                bare::GPIO::setFunction(RPiResetPin, bare::GPIO::Function::Output);
                bare::Timer::usleep(10000);
                bare::GPIO::setPin(RPiResetPin, false);
                bare::Timer::usleep(100000);
                bare::GPIO::setPin(RPiResetPin, true);
                bare::Timer::usleep(100000);
                bare::GPIO::setFunction(RPiResetPin, bare::GPIO::Function::Input);
            } else if (array[1] == "esp") {
                showMessage(MessageType::Info, "Resetting ESP\n");
                bare::restart();
            } else {
                showMessage(MessageType::Error, "invalid test command\n");
            }
        } else if (array[0] == "test") {
            if (array.size() < 2) {
                showMessage(MessageType::Error, "need a test command\n");
            } else if (array[1] == "spi") {
                bare::Serial::printf("SPI test\n");
                testSPI();
            } else {
                showMessage(MessageType::Error, "invalid test command\n");
            }
        } else {
            return false;
        }
        return true;
    }
};

MyShell myShell;

// gets called when WiFiManager enters configuration mode
void configModeCallback (WiFiManager *myWiFiManager)
{
    bare::Serial::printf("Entered config mode: IP=%s, SSID=%s\n", WiFi.softAPIP().toString().c_str(), myWiFiManager->getConfigPortalSSID().c_str());

    //entered config mode, make led toggle faster
    blinker.setRate(ConfigRate);
}

void startWifi()
{
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
        delay(1000);
    }

    //if you get here you have connected to the WiFi
    bare::Serial::printf("Wifi connected, IP=%s\n", WiFi.localIP().toString().c_str());
    blinker.setRate(ConnectedRate);
}

void setup()
{
    bare::Serial::init(115200);
    
    bare::Serial::printf("\n\nPlacid Wifi v0.1\n\n");

    blinker.setRate(IdleRate);
    myShell.connected();
}

void loop()
{
    uint8_t c;
    bare::Serial::Error error = bare::Serial::read(c);
    if (error == bare::Serial::Error::OK) {
        myShell.received(c);
    } else if (error != bare::Serial::Error::NotReady) {
        bare::Serial::puts("*** Serial Read Error\n");
    }
}
