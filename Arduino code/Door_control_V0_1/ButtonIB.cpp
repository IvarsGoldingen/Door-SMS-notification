//This needs to be included in the library so it can access standard arduino functions
#include "Arduino.h"
#include "ButtonIB.h"

//A constructor for the class
ButtonIB::ButtonIB(byte pin, void(*cbPressed)(void)){
  buttonPressedCb = cbPressed;
  button_pin = pin;
  pinMode(button_pin, INPUT_PULLUP);
}

void ButtonIB::loop(){
  if (checkButtonTrigger()){
    //callback to main function when button pressed
    buttonPressedCb();
  }
}

/*
 * Returns true when button is pressed
 */
bool ButtonIB::checkButtonTrigger(){
  if(!digitalRead(button_pin)){
    //Button has been pressed, input has gone low
    if((millis() - timeOfPress) >= DEBOUNCE_TIME){
      //check if the debounce time since the last press has passed
      //register the last time the button has been pressed
      timeOfPress = millis();
      //If input goes low, that means the button has been pressed
      return true;
    }
  }
  return false;
}
