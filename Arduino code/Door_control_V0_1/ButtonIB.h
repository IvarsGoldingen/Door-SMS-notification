//This prevents problems if somehow this calls was implemented twice in the same scetch
#ifndef ButtonIB_h
#define ButtonIB_h

/*
 * Class for cntrolling inputs, specifically for buttons
 * connect button to GND, because pullup used on input
 */

 //This needs to be included in the library so it can access standard arduino functions
#include "Arduino.h"

class ButtonIB{
  private:
  
    //Class member variables
    //Time restriction between registered button pushes
    const int DEBOUNCE_TIME = 400;
    //Input pin of the button
    byte button_pin;
    //stores the last time the button press was registered. Used for debouncing purposes
    long timeOfPress = 0;
    /*
     * Function called in each loop to check if the button has been pressed
     * Returns true when the button has been let go of
     */
     bool checkButtonTrigger();
     //A pointer to a function in the main scetch. Triggered when button is pressed
    void (*buttonPressedCb)(void) = 0;
  public:
    /*
    *A constructor for the class
    *pin = the input pin which is connected to the button
    **cbPressed = callback to the main function when button has been pressed
    */
    ButtonIB(byte pin, void(*cbPressed)(void));
    /*
     * Loop function of the class
    */ 
    void loop();
};

#endif
