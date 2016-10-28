#include <Arduino.h>

void setup();
void loop();
#line 1 "src/sketch.ino"

#define LED_PIN 13

void setup()
{
    pinMode(LED_PIN, OUTPUT);
Serial.begin(9600);
}

void loop()
{
Serial.println("Hello");

    digitalWrite(LED_PIN, HIGH);
    delay(100);
    digitalWrite(LED_PIN, LOW);
    delay(900);
}
