#include "Sim.h"

//GIT KRAKEN TEST
//Pins that will be used for the software serial to communicate with the sim900 chip
const byte SW_SERIAL_PIN1 = 2;
const byte SW_SERIAL_PIN2 = 3;

//Minimum time necessary for the button press to be registered as an event.
const int MINIMUM_SWITCH_TIME = 100;
//Set minimum time between door events to 5 minutes = 5m * 60s * 1000ms = 300 000
const float DELAY_FOR_DOOR_EVENTS = 300000;

/**Status indication values**/
//Green led output. Const on SIM900 ready, blinking setting up
const int STATUS_LED_O = 8;
//Const on, when sim900 fails to setup
const int ERROR_LED_O = 9;

//Door trigger values
const byte DOOR_SENSOR_P = 12;
bool doorTriggered = false;
unsigned long doorMoveTime = 0;
//Made true when door is triggered
//when true the sim chip will get turned on, sms sent, then turned off
bool sending = false;

/*********************************************************************************
callback from the SIM library
must be declared before the contructur so the function is visible to it
Will inform the main scetch of finished actions and errors
possible status updates from Sim lib to main sketch
    const byte INIT_FINISHED = 1;
    const byte INIT_FAILED = 2;
    const byte SMS_SENT = 3;
    const byte SMS_SEND_FAILED = 4;
    const byte TUREND_OFF = 5;
*/
void updateCallback(byte message){
  Serial.println("CB");
  if (sending){
    //if sending started use info from SIM to send the message
    sendSequence(message);
  }
}
//Create a Sim object to control the Sim900 chip
Sim sim(SW_SERIAL_PIN1,SW_SERIAL_PIN2,STATUS_LED_O,ERROR_LED_O, updateCallback);

void setup() {
  //Serial setup
  //Initiate serial with the computer
  Serial.begin(9600);
  //PIN setup
  pinMode(DOOR_SENSOR_P, INPUT_PULLUP);
  //Turn the sim module of on startup
  digitalWrite(7, LOW);
  Serial.println("ON");
}


void loop() {
  manualControl();
  sim.loop();
  /*
  if(!sending){
    //if we are not sending already, check for door trigger
    if(checkDoorOpen()){
      //start sending of SMS
      if (timePassed(doorMoveTime) > DELAY_FOR_DOOR_EVENTS){
        //if the time betwen SMS sends has passed
        sending = true;
      }
    } 
  }
  */
}
//turn off sending of SMS and turns the sim chip of
void stopSim(){
  sim.off();
  sending = false;
}

void sendSequence(byte simStatus){
  //the first status mesages from sim will be about initialization finished or failed
  switch(simStatus){
    case sim.INIT_FINISHED:
      Serial.println("SIM INIT SUCCESS");
      //if init has finished send the sms
      sim.sendMessage("Init finished");
      break;
    case sim.INIT_FAILED:
      //if init failed 
      stopSim();
      //TODO: perharps try resending
      //TODO: distinguish errors that mean no SIM card etc, and errors that might allow for resending
      Serial.println("Init failed");
      break;
    case sim.SMS_SENT:
      //if init succesfull turn of sim module
      Serial.println("SMS_SUCCESS"); 
      stopSim();
      break;
    case sim.SMS_SEND_FAILED:
      //if sending failed 
      stopSim();
      Serial.println("send failed");
      break;
    case sim.TUREND_OFF:
      break;
    default:
      Serial.println("CB UM");
      break;
  }
}


//returns true if door switch has been triggered
bool checkDoorOpen(){
  if (!doorTriggered){
    //if the door has not yet been triggered
    // read door sensor. Reverso so true is for triggered
    if (timePassed(doorMoveTime) > MINIMUM_SWITCH_TIME){
      //debounce time has passed
      if (digitalRead(DOOR_SENSOR_P)){
        //Door opened
        Serial.println("Door open");
        //if the door sensor has been activated, chech the time
        doorMoveTime = millis();
        doorTriggered = true;
        //send true so SMS sending starts
        return true;
      }
    }
  } else if (!digitalRead(DOOR_SENSOR_P)){
    //if the door has been closed again allow for the next listen
    doorTriggered = false;
  }
  return false;
}


//returns true if door switch has been triggered
bool checkDoorTriggered(){
  if (!doorTriggered){
    // read door sensor. Reverso so true is for triggered
    if (!digitalRead(DOOR_SENSOR_P)){
      Serial.println("Triggered");
      //if the door sensor has been activated, chech the time
      doorMoveTime = millis();
      doorTriggered = true;
    }
  } else {
    //wait some time before registering the event
    if (timePassed(doorMoveTime) > MINIMUM_SWITCH_TIME){
      //minimu lenght of press 100ms
       if (digitalRead(DOOR_SENSOR_P)){
        //if the button has been let go, allow listening for the next trigger
        doorTriggered = false;
        //togle test LED
        //digitalWrite(TEST_LED_O, !digitalRead(TEST_LED_O));
      }
      return true;
    }
  }
  return false;
}

//calculates time passed from the time sent to the function
unsigned long timePassed(unsigned long time){
  return millis() - time;
}


//allows control of sim with serial port
void manualControl(){
  if(Serial.available()){
    Serial.println("man");
    while(Serial.available()){
      char pcChar = Serial.read();
      if (pcChar == 'a'){
        //0x1A is needed to send an SMS, Y used to be able to send it through terminal
        sim.setupSim();
      } else if (pcChar == 's') {
        Serial.println("off");
        sim.off();
      } else if (pcChar == 'd') {
        sim.on();
      } else if (pcChar == 'w') {
        sim.sendMessage("Test 3");
      } else if (pcChar == 'e') {
        //Initiate sim setup
        sim.setupSim();
        //The scetch will auomatically send the message when sim has been initialized
        sending = true;
      }
    }
  }
}
