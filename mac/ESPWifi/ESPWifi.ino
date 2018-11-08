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

#pragma once

// Ports
//
//      D5 - SS
//      D6 - MISO
//      D7 - MOSI
//      D8 - SCK
//

#include <FS.h>                   //this needs to be first, or it all crashes and burns...
#include <ESP8266WiFi.h>          //https://github.com/esp8266/Arduino

//needed for library
#include <DNSServer.h>
#include <ESP8266WebServer.h>
#include <WiFiManager.h>          //https://github.com/tzapu/WiFiManager

#include <ESP8266HTTPClient.h>

#include <time.h>
#include <assert.h>

const uint32_t LoopRate = 10; // loop rate in ms

const uint32_t ScrollRate = 200; // scroll rate in ms

// Number of ms LED stays off in each mode
constexpr uint32_t BlinkSampleRate = 10;
const uint32_t ConnectingRate = 400;
const uint32_t ConfigRate = 100;
const uint32_t ConnectedRate = 1900;

const uint32_t DebugPrintRate = 1;

const uint32_t LIGHT_SENSOR = A0;

const uint32_t MaxAmbientLightLevel = 800;
const uint32_t NumberOfBrightnessLevels = 64;
constexpr uint32_t MaxBrightness = 80;
constexpr uint32_t MinBrightness = 3;
bool needBrightnessUpdate = false;

constexpr uint8_t NumBrightnessSamples = 5;
constexpr uint32_t BrightnessSampleRate = 100;


int8_t curTemp, highTemp, lowTemp;
uint32_t currentTime = 0;

uint16_t timeCounterSecondsRemaining;

bool connecting = true;
bool needWeatherLookup = false;
bool needsUpdateDisplay = false;
bool showWelcomeMessage = true;

const char* weatherCity = "Los_Altos";
const char* weatherState = "CA";
constexpr char* WUKey = "5bc1eac6864b7e57";

enum class DisplayState { Time, Day, Date, CurrentTemp, HighTemp, LowTemp, CountdownTimerAsk, CountdownTimerCount, CountdownTimerDone };
DisplayState displayState = DisplayState::Time;

const char startupMessage[] = "Office Clock v1.0";

Max72xxPanel matrix = Max72xxPanel(SS, 4, 1);

class ClockDisplay
{
public:
  ClockDisplay()
  {
    pinMode(SS, OUTPUT);
    digitalWrite(SS, LOW);

    matrix.setFont(&OfficeClock_8x8_Font8pt);
    matrix.setTextWrap(false);
    
    matrix.setIntensity(0); // Use a value between 0 and 15 for brightness

    matrix.setPosition(0, 0, 0); // The first display is at <0, 0>
    matrix.setPosition(1, 1, 0); // The second display is at <1, 0>
    matrix.setPosition(2, 2, 0); // The third display is at <2, 0>
    matrix.setPosition(3, 3, 0); // And the last display is at <3, 0>

    matrix.setRotation(0, 1);    // The first display is position upside down
    matrix.setRotation(1, 1);    // The first display is position upside down
    matrix.setRotation(2, 1);    // The first display is position upside down
    matrix.setRotation(3, 1);    // The same hold for the last display

    clear();
  }

  void clear()
  {
      matrix.fillScreen(LOW);
      matrix.write(); // Send bitmap to display
  }
  
    void setBrightness(float level)
    {
    level *= 15;
    if (level > 15) {
      level = 15;
    } else if (level < 0) {
      level = 0;
    }
      matrix.setIntensity(level);
    }
        
    void setString(const String& string, bool colon = false, bool pm = false)
    {
    String s = string;
    if (string.length() >= 2 && colon) {
      s = s.substring(0, 2) + ":" + s.substring(2);
    }
    s.trim();  
    
  // center the string
    int16_t x1, y1;
    uint16_t w, h;
    matrix.getTextBounds((char*) s.c_str(), 0, 0, &x1, &y1, &w, &h);
    
    matrix.setCursor((matrix.width() - w) / 2, matrix.height() - (h + y1));
    matrix.fillScreen(LOW);
    matrix.print(s);
    matrix.write(); // Send bitmap to display
    }

  bool scrollString(const String& s, int32_t offset)
  {
    // Make scroll start offscreen
    offset -= matrix.width();
    
    int16_t x1, y1;
    uint16_t w, h;
    matrix.getTextBounds((char*) s.c_str(), 0, 0, &x1, &y1, &w, &h);
    if (offset >= matrix.width() + w) {
      return false;
    }

    matrix.fillScreen(LOW);

    matrix.setCursor(-offset, matrix.height() - (h + y1));
    matrix.print(s);
    matrix.write(); // Send bitmap to display
    return true;
  }
    
 private:
  int _characterSpace = 1;
};

ClockDisplay clockDisplay;

class Blinker
{
public:
    Blinker() { _ticker.attach_ms(BlinkSampleRate, blink, this); }
    
    void setRate(uint32_t rate) { _rate = (rate + (BlinkSampleRate / 2)) / BlinkSampleRate; }
    
private:
    static void blink(Blinker* self)
    {
        if (self->_count == 0) {
            digitalWrite(BUILTIN_LED, LOW);
        } else if (self->_count == 1){
            digitalWrite(BUILTIN_LED, HIGH);
        }
        if (++self->_count >= self->_rate) {
            self->_count = 0;
        }
    }
    
    Ticker _ticker;
    uint32_t _rate = 10; // In 10 ms units
    uint32_t _count = 0;
};

Blinker blinker;

class BrightnessManager
{
public:
    BrightnessManager()
    {
      _ticker.attach_ms(BrightnessSampleRate, compute, this);
    }
    
    uint8_t brightness() const { return _currentBrightness; }
    
private:
    static void compute(BrightnessManager* self) { self->computeBrightness(); }

    void computeBrightness()
    {
        uint32_t ambientLightLevel = analogRead(LIGHT_SENSOR);
        m8r::cout << "AnalogRead:'" << ambientLightLevel << "'\n";
        
        uint32_t brightnessLevel = ambientLightLevel;
        if (brightnessLevel > MaxAmbientLightLevel) {
            brightnessLevel = MaxAmbientLightLevel;
        }
    
        // Make brightness level between 1 and NumberOfBrightnessLevels
        brightnessLevel = (brightnessLevel * NumberOfBrightnessLevels + (MaxAmbientLightLevel / 2)) / MaxAmbientLightLevel;
        if (brightnessLevel >= NumberOfBrightnessLevels) {
            brightnessLevel = NumberOfBrightnessLevels - 1;
        }
        
        _brightnessAccumulator += brightnessLevel;        

        if (++_brightnessSampleCount >= NumBrightnessSamples) {
            _brightnessSampleCount = 0;
            uint32_t brightness = (_brightnessAccumulator + (NumBrightnessSamples / 2)) / NumBrightnessSamples;
            _brightnessAccumulator = 0;

            if (brightness != _currentBrightness) {
                _currentBrightness = brightness;
                m8r::cout << "Setting brightness:'" << _currentBrightness << "'\n";
                
                float v = _currentBrightness;
                v /= NumberOfBrightnessLevels - 1;
                clockDisplay.setBrightness(v);
                m8r::cout << "    level:'" << v << "'\n";
            }
        }
    }

    uint8_t _currentBrightness = 255;
    uint32_t _brightnessAccumulator = 0;
    uint8_t _brightnessSampleCount = 0;
    Ticker _ticker;
};

BrightnessManager brightnessManager;

static void decimalByteToString(uint8_t v, char string[2], bool showLeadingZero)
{
    string[0] = (v < 10 && !showLeadingZero) ? ' ' : (v / 10 + '0');
    string[1] = (v % 10) + '0';
}

static void tempToString(char c, int8_t t, String& string)
{
    // string = String(c);
    // if (t < 0) {
    //     t = -t;
    //     string[1] = '-';
    // } else
    //     string[1] = ' ';
    //
    // decimalByteToString(t, &string[2], false);
}

static void showChars(const String& string, bool pm, bool colon)
{
    static String lastStringSent;
    if (string == lastStringSent) {
        return;
    }
    lastStringSent = string;
    
    assert(string.length() == 4);
  clockDisplay.setString(string, colon, pm);
        
    // bool blank = false;
    // bool endOfString = false;
    //
    // for (uint8_t stringIndex = 0, outputIndex = 0; outputIndex < 4; ++stringIndex, ++outputIndex, dps <<= 1) {
    //     if (string.c_str()[stringIndex] == '\0')
    //         endOfString = true;
    //
    //     if (endOfString)
    //         blank = true;
    //
    //     uint8_t glyph1, glyph2 = 0;
    //     bool hasSecondGlyph = m8r::SevenSegmentDisplay::glyphForChar(blank ? ' ' : string.c_str()[stringIndex], glyph1, glyph2);
    //
    //     shiftReg.send(glyph1 | ((dps & 0x08) ? 0x80 : 0), 8);
    //
    //     if (hasSecondGlyph && outputIndex != 3) {
    //         ++outputIndex;
    //         dps <<= 1;
    //         shiftReg.send(glyph2 | ((dps & 0x08) ? 0x80 : 0), 8);
    //     }
    // }
    // shiftReg.latch();
}

void updateDisplay()
{
    String string = "EEEE";
    bool pm = false;
    bool colon = false;
    
    switch (displayState) {
        case DisplayState::Date:
        case DisplayState::Day:
        case DisplayState::Time: {
            struct tm* timeinfo = localtime(reinterpret_cast<time_t*>(&currentTime));
            
            switch (displayState) {
                case DisplayState::Day:
                    // RTCBase::dayString(t.day, string);
                    break;
                case DisplayState::Date:
                    // decimalByteToString(t.month, string, false);
                    // decimalByteToString(t.date, &string[2], false);
                    break;
                case DisplayState::Time: {
                    colon = true;
                    uint8_t hours = timeinfo->tm_hour;
                    if (hours == 0) {
                        hours = 12;
                    } else if (hours >= 12) {
                        pm = true;
                        if (hours > 12) {
                            hours -= 12;
                        }
                    }
                    if (hours < 10) {
                        string = " ";
                    } else {
                        string = "";
                    }
                    string += String(hours);
                    if (timeinfo->tm_min < 10) {
                        string += "0";
                    }
                    string += String(timeinfo->tm_min);
                    break;
                }
                default:
                    break;
            }
            break;
        }
        case DisplayState::CurrentTemp:
            tempToString('C', curTemp, string);
            break;
        case DisplayState::HighTemp:
            tempToString('H', highTemp, string);
            break;
        case DisplayState::LowTemp:       
            tempToString('L', lowTemp, string);
            break;
        case DisplayState::CountdownTimerAsk:       
            string = "cnt?";
            break;
        case DisplayState::CountdownTimerCount:
            // decimalByteToString(m_timeCounterSecondsRemaining / 60, string, false);
            // decimalByteToString(m_timeCounterSecondsRemaining % 60, &string[2], true);
            break;
        case DisplayState::CountdownTimerDone:
            string = "done";
            break;
        default:
            break;
    }
    
    showChars(string, pm, colon);
}

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

class MyJsonListener : public JsonListener
{
public:
    virtual ~MyJsonListener() { }
    
    virtual void key(String key) override
    {
        if (key == "local_epoch") {
            _waitingForLocalEpoch = true;
        } else if (key == "local_tz_offset") {
            _waitingForLocalTZOffset = true;
        }
    }
    
    virtual void value(String value) override
    {
        if (_waitingForLocalEpoch) {
            _waitingForLocalEpoch = false;
            _localEpoch = value.toInt();
        } else if (_waitingForLocalTZOffset) {
            _waitingForLocalTZOffset = false;
            _localTZOffset = value.toInt();
        } 
    }
    
    virtual void whitespace(char c) override { }
    virtual void startDocument() override { }
    virtual void endArray() override { }
    virtual void endObject() override { }
    virtual void endDocument() override { }
    virtual void startArray() override { }
    virtual void startObject() override { }
    
    uint32_t localEpoch() const { return _localEpoch; }
    int32_t localTZOffset() const { return _localTZOffset; }
    
private:
    bool _waitingForLocalEpoch = false;
    uint32_t _localEpoch = 0;
    bool _waitingForLocalTZOffset = false;
    int32_t _localTZOffset = 0;
};

void setCheckWeather()
{
    needWeatherLookup = true;
}
void checkWeather()
{
    HTTPClient http;
    m8r::cout << "Getting weather and time feed...\n";

    String wuURL;
    wuURL += "http://api.wunderground.com/api/";
    wuURL += WUKey;
    wuURL +="/conditions/forecast/q/";
    wuURL += weatherState;
    wuURL += "/";
    wuURL += weatherCity;
    wuURL += ".json?a=";
    wuURL += millis();
    
    m8r::cout << "URL='" << wuURL << "'\n";
    
    http.begin(wuURL);
    int httpCode = http.GET();

    if(httpCode > 0) {
        m8r::cout << "    got response: " << httpCode << "\n";

         if(httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            m8r::cout << "Got payload, parsing...\n";
            JsonStreamingParser parser;
            MyJsonListener listener;
            parser.setListener(&listener);
            for (int i = 0; i < payload.length(); ++i) {
                parser.parse(payload.c_str()[i]);
            }
            
            currentTime = listener.localEpoch() + (listener.localTZOffset() * 3600 / 100);
            needsUpdateDisplay = true;
        }
    } else {
        m8r::cout << "[HTTP] GET... failed, error: " << http.errorToString(httpCode) << "\n";
    }

    http.end();

    static Ticker ticker;
    
    // Check one minute past the hour. That way if daylight savings changes, we catch it sooner
    int32_t timeToNextHour = (60 * 60) - (static_cast<int32_t>(currentTime % (60 * 60))) + 60;
    ticker.once(timeToNextHour, setCheckWeather);
    
    m8r::cout << "Time set to:" << ctime(reinterpret_cast<time_t*>(&currentTime)) << ", next setting in " << timeToNextHour << " seconds\n";
}

void setup()
{
  Serial.begin(115200);
  
  m8r::cout << "\n\nOffice Clock v1.0\n\n";
      
  clockDisplay.setBrightness(0);
  clockDisplay.setString(".......");
    
    //set led pin as output
  pinMode(BUILTIN_LED, OUTPUT);
  blinker.setRate(ConnectingRate);

    if (SPIFFS.begin()) {
        Serial.println("mounted file system");
    } else {
        Serial.println("failed to mount FS");
    }
    
    //WiFiManager
    //Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wifiManager;

    //reset settings - for testing
    //wifiManager.resetSettings();
    
    //set callback that gets called when connecting to previous WiFi fails, and enters Access Point mode
    wifiManager.setAPCallback(configModeCallback);
    
    //fetches ssid and pass and tries to connect
    //if it does not connect it starts an access point with the specified name
    //here  "AutoConnectAP"
    //and goes into a blocking loop awaiting configuration
    if (!wifiManager.autoConnect()) {
        Serial.println("failed to connect and hit timeout");
        //reset and try again, or maybe put it to deep sleep
        ESP.reset();
        delay(1000);
    }

    //if you get here you have connected to the WiFi
    m8r::cout << "Wifi connected, IP=" << WiFi.localIP() << "\n";
  blinker.setRate(ConnectedRate);
  needWeatherLookup = true;
}

void loop()
{
    // Only run the loop every LoopRate ms
    static uint32_t lastMillis = 0;
    uint32_t nextMillis = millis();
    if (nextMillis < lastMillis + LoopRate) {
        return;
    }
    lastMillis = nextMillis;
    
    // Tick the current time if needed
    static uint32_t lastSecondTick = 0;
    if (lastSecondTick == 0) {
        lastSecondTick = nextMillis;
    } else if (lastSecondTick <= nextMillis + 1000) {
        lastSecondTick += 1000;
        currentTime++;
        needsUpdateDisplay = true;
    }
    
    // Debug printing
    static uint32_t nextDebugPrint = 0;
    if(nextDebugPrint < currentTime) {
        nextDebugPrint = currentTime + DebugPrintRate;
        
        // m8r::cout << "***** current brightness=" << brightnessManager.brightness() << "\n";
        // m8r::cout << "***** Ambient Light Level = " << analogRead(LIGHT_SENSOR) << "\n";
    }
    
    // Get the time and weather if needed
    if (needWeatherLookup) {
        checkWeather();
        needWeatherLookup = false;
    }
    
    if (needsUpdateDisplay && !showWelcomeMessage) {
        updateDisplay();
        needsUpdateDisplay = false;
    }

    if (showWelcomeMessage) {
        // Scroll the welcome message
    static int32_t welcomeScrollOffset = 0;
    static uint32_t welcomeScrollCount = 1;
    static uint32_t welcomeScrollDelay = 0;

    if (++welcomeScrollDelay < 6) {
      return;
    }

    welcomeScrollDelay = 0;

    if (!clockDisplay.scrollString(startupMessage, welcomeScrollOffset++)) {
      welcomeScrollOffset = 0;
      if (--welcomeScrollCount == 0) {
        showWelcomeMessage = false;
        return;
      }        
        }
    }
}
