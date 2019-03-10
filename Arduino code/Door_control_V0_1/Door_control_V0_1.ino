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

//Manual control characters
//to set the time use this format x19/03/09,11:56:55+08,<yy/MM/dd,hh:mm:ss±zz> 
const char SET_TIME_CHAR = 'x';
//returns the time stored in the sim module's RTC
const char GET_TIME_CHAR = 'y';

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

/*********************************************************************************
callback from the SIM library
must be declared before the contructur so the function is visible to it
sends time from SIM modules RTC
*/
void timeCallback(String simTime){
  Serial.println(simTime);
  //-48 because the received number is from ASCII table not numeric
  byte year = ((simTime[0] - 48) * 10) + (simTime[1] - 48);
  Serial.println(year);
  byte month = ((simTime[3] - 48) * 10) + (simTime[4] - 48);
  Serial.println(month);
  byte day = ((simTime[6] - 48) * 10) + (simTime[7] - 48);
  Serial.println(day);
  byte hour = ((simTime[9] - 48) * 10) + (simTime[10] - 48);
  Serial.println(hour);
  byte minute = ((simTime[12] - 48) * 10) + (simTime[13] - 48);
  Serial.println(minute);
  byte second = ((simTime[15] - 48) * 10) + (simTime[16] - 48);
  Serial.println(second);
  byte weekDay = calcDayOfWeek (year, month, day);
  Serial.println(weekDay);
  if (sending){
    //delay(1000);
    test(simTime);  
  }
}

//Create a Sim object to control the Sim900 chip
Sim sim(SW_SERIAL_PIN1,SW_SERIAL_PIN2,STATUS_LED_O,ERROR_LED_O, updateCallback, timeCallback);


void test(String simTime){
  sim.sendMessage(simTime);  
}
 
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

//When status messages come in from the SIM module, make next steps for sending
void sendSequence(byte simStatus){
  //the first status mesages from sim will be about initialization finished or failed
  switch(simStatus){
    case sim.INIT_FINISHED:
      Serial.println("SIM INIT SUCCESS");
      //if init has finished, get the time 
      sim.getTime();
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
      } else if (pcChar == 'o') {
        Serial.println("off");
        sim.off();
      } else if (pcChar == 'i') {
        sim.on();
      } else if (pcChar == 'w') {
        sim.sendMessage("Test 3");
      } else if (pcChar == 'e') {
        //Initiate sim setup
        sim.setupSim();
        //The scetch will auomatically send the message when sim has been initialized
        sending = true;
      } else if (pcChar == SET_TIME_CHAR){
        //delay to receive all the sent bytes from serial
        delay(10);
        //20 bytes needed for setting of the time
        byte timeBuffer[20];
        byte timeBufferCount = 0;
        //if setting the time, read the whole message from the serial buffer
        while(Serial.available()){
           timeBuffer[timeBufferCount++] = Serial.read();
           //with baud 9600, byte lenght about 1 ms, so wait for 5 do make sure all symbols are read before exiting the loop
           delay(10);
        }
        if (timeBufferCount == 20){
          //the correct number of bytes is 20 for setting the time
          //TODO: check the time format deeper or use reading answer messages from SIM900
          sim.setTime(timeBuffer);
        } else {
          Serial.println("TIME FORMAT ERROR");
          Serial.println(timeBufferCount);
        }
      } else if (pcChar == GET_TIME_CHAR){
        sim.getTime();
      }
    }
  }
}

//calculates the day of week from date
byte calcDayOfWeek (byte y, byte m, byte d) {
  // Old mental arithmetic method for calculating day of week
  // adapted for Arduino, for years 2000~2099
  // returns 1 for Sunday, 2 for Monday, etc., up to 7 for Saturday
  // for "bad" dates (like Feb. 30), it returns 0
  // Note: input year (y) should be a number from 0~99
  if (y > 99) return 0; // we don't accept years after 2099
  // we take care of bad months later
  if (d < 1) return 0; // because there is no day 0
  byte w = 6; // this is a magic number (y2k fix for this method)
  // one ordinary year is 52 weeks + 1 day left over
  // a leap year has one more day than that
  // we add in these "leftover" days
  w += (y + (y >> 2));
  // correction for Jan. and Feb. of leap year
  if (((y & 3) == 0) && (m <= 2)) w--;
  // add in "magic number" for month
  switch (m) {
    case 1:  if (d > 31) return 0; w += 1; break;
    case 2:  if (d > ((y & 3) ? 28 : 29)) return 0; w += 4; break;
    case 3:  if (d > 31) return 0; w += 4; break;
    case 4:  if (d > 30) return 0; break;
    case 5:  if (d > 31) return 0; w += 2; break;
    case 6:  if (d > 30) return 0; w += 5; break;
    case 7:  if (d > 31) return 0; break;
    case 8:  if (d > 31) return 0; w += 3; break;
    case 9:  if (d > 30) return 0; w += 6; break;
    case 10: if (d > 31) return 0; w += 1; break;
    case 11: if (d > 30) return 0; w += 4; break;
    case 12: if (d > 31) return 0; w += 6; break;
    default: return 0;
  }
  // then add day of month
  w += d;
  // there are only 7 days in a week, so we "cast out" sevens
  while (w > 7) w = (w >> 3) + (w & 7);
  return w;
}
