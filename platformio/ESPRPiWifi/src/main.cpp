#include <Arduino.h>

void setup() {
    pinMode(LED_BUILTIN, OUTPUT);  // initialize onboard LED as output
}

void loop() {
    digitalWrite(LED_BUILTIN, HIGH);  // turn off LED with voltage HIGH
    delay(1000);
    digitalWrite(LED_BUILTIN, LOW);   // turn on LED with voltage LOW
    delay(5);
}