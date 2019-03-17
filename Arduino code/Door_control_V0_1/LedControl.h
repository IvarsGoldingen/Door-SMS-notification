//This prevents problems if somehow this calls was implemented twice in the same scetch
#ifndef LedControl_h
#define LedControl_h

//This needs to be included in the library so it can access standard arduino functions
#include "Arduino.h"

class LedControl{
  private:
    //Class member variables
    //the output pin used for the led
    byte ledPin;
    //period of the blink
    unsigned int blinkTime;
    //Variable to check if blinking is neccessary
    bool blinkOn = false;
    //Used to time when the led needs to be turned on or off
    unsigned long timeOfLastSwitch = 0;
  public:
    //A constructor for the class
    /*
     * pin = pin on the arduino on which the LED is connected
     */
    LedControl(byte pin);
    //loop function necessary when led is set to blinking mode
    void loop();
    /*
     * Sets the ouptut to turn on/off
     * blinkTime = time between switches
     */
    void ledBlink(unsigned int blinkTime);
    //turns LED off, stops blinker
    void ledOff();
    //turns led on, stops blinker
    void ledOn();
};

#endif
