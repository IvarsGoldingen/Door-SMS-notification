//This needs to be included in the library so it can access standard arduino functions
#include "Arduino.h"
#include "LedControl.h"

//A constructor for the class
LedControl::LedControl(byte pin){
  ledPin = pin;
  pinMode(pin, OUTPUT);
}

void LedControl:: ledOn(){
  digitalWrite(ledPin, HIGH);
  blinkOn = false;
}

void LedControl:: ledOff(){
  digitalWrite(ledPin, LOW);
  blinkOn = false;
}

void LedControl::ledBlink(unsigned int timeMs){
  blinkTime = timeMs;
  blinkOn = true;
  timeOfLastSwitch = millis();
}

void LedControl::loop(){
  if (blinkOn){
    if (millis() - timeOfLastSwitch > blinkTime){
      timeOfLastSwitch = millis();
      digitalWrite(ledPin, !digitalRead(ledPin));
    }
  }
}
