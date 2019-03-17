#include "Sim.h"
#include "ButtonIB.h"
#include "LedControl.h"

/**Status indication values**/
//Green led output. Const on SIM900 ready, blinking setting up
const int STATUS_LED_O = 8;
//Const on, when sim900 fails to setup
const int ERROR_LED_O = 9;

//Pins that will be used for the software serial to communicate with the sim900 chip
const byte SW_SERIAL_PIN1 = 2;
const byte SW_SERIAL_PIN2 = 3;

//Pin used for send mode change
const byte MODE_BUTTON = 10;
//Pin used for signaling of the current send mode of the system
const byte MODE_LED = 13;

//Door trigger values
const byte DOOR_SENSOR_P = 12;

//Minimum time necessary for the door open to be registered as an event.
const int MINIMUM_SWITCH_TIME = 1000;
//Set minimum time between door events 
//5 minutes = 5m * 60s * 1000ms = 300 000
const float DELAY_FOR_DOOR_EVENTS = 60000;

//Time constraints for when the sms is being sent
//returns 1 for Sunday, 2 for Monday, etc., up to 7 for Saturday
//the 2nd youngest bit represents Sunday, the oldest bit represents Saturday, the youngest bit does not represent anything
const byte SEND_WEEKDAYS = 0b01111100;
//Time of day from which SMS are sent on appropriate weekdays
const byte SEND_TIME_H_START = 8;
//Time of day from which SMS are np longer sent on approptiate weekdays
const byte SEND_TIME_H_END = 17;

//Manual control characters for serial port
//to set the time use this format x19/03/09,11:56:55+08,<yy/MM/dd,hh:mm:ss±zz> +08 for gmt+2,
//module must be on and initialized, that must be done mannualy before
const char SET_TIME_CHAR = 'x';
//returns the time stored in the sim module's RTC
const char GET_TIME_CHAR = 'y';
//change modes from don't send, send always, send depending on time
const char CHANGE_MODE_CHAR = 'm';
//turn module on and set PIN code
const char INIT_CHAR = 'e';
//turn module off
const char OFF_CHAR = 'o';
//sends test message when module is already initialized
const char TEST_MESSAGE_CHAR = 'w';
//full sequence, on, init, get time, send message, turn off
const char SEND_SEQUENCE_CHAR = 'a';

// 3 modes for sending
//Send depending on time
const byte SEND_TIME = 1;
//Send allways
const byte SEND_ALWAYS = 2;
//don't send
const byte SEND_OFF = 3;

//variable of keeping track of the sending mode
byte sendMode = SEND_TIME;


bool doorTriggered = false;
//variable to store time since the last door moved, used for debounce
//set to negative MINIMUM_SWITCH_TIME so works instantly on startup
long doorMoveTime = -MINIMUM_SWITCH_TIME;
////variable to store time since the last door moved, used for setting delay between SMS sends 
long doorTriggerTime = -DELAY_FOR_DOOR_EVENTS;

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
sends time from SIM module's RTC
*/
void timeCallback(String simTime){
  Serial.println(simTime);
  if (sending){
    //if sending is currently in progress then continue with sending the sms
    evaluateSend(simTime);  
  }
}

//Create a Sim object to control the Sim900 chip
Sim sim(SW_SERIAL_PIN1,SW_SERIAL_PIN2,STATUS_LED_O,ERROR_LED_O, updateCallback, timeCallback);

void modeButtonPressed(){
  switchSendMode();
}

//A button object for receiving the mode change pressses
ButtonIB modeButton(MODE_BUTTON, modeButtonPressed);

LedControl modeLed(MODE_LED);

//Evaluates if sending of SMS is neccessary depending on send mode and time
//TODO: check only time here, and check for send off earlier
void evaluateSend(String simTime){
  if (sendMode == SEND_OFF){
    Serial.println("Sending off");
    stopSim();
  } else if (sendMode == SEND_TIME){
    //-48 because the received number is from ASCII table not numeric
    byte year = ((simTime[0] - 48) * 10) + (simTime[1] - 48);
    byte month = ((simTime[3] - 48) * 10) + (simTime[4] - 48);
    byte day = ((simTime[6] - 48) * 10) + (simTime[7] - 48);
    byte hour = ((simTime[9] - 48) * 10) + (simTime[10] - 48);
    //byte minute = ((simTime[12] - 48) * 10) + (simTime[13] - 48);
    //byte second = ((simTime[15] - 48) * 10) + (simTime[16] - 48);
    byte weekDay = calcDayOfWeek(year, month, day);
    bool sendSMS = false;
    Serial.println(SEND_WEEKDAYS & (0b00000001 << weekDay));
    if((SEND_WEEKDAYS & (0b00000001 << weekDay)) > 0){
      Serial.println("Weekday");
      if(hour >= SEND_TIME_H_START &&
        hour < SEND_TIME_H_END){
        Serial.println("Working time");
        sendSMS = true;
      } else {
        Serial.println("Past working time");
      }
    } else {
      Serial.println("Weekend");
    }
    if (sendSMS){
      //TODO: comment
      Serial.println("Sending");
      sim.sendMessage("Door triggered\r" + simTime);
    } else {
      stopSim();
      Serial.println("Not sending");
    }
  } else if (sendMode == SEND_ALWAYS){
    Serial.println("Send allways");
    sim.sendMessage("Door triggered\r" + simTime);
  }
}

void sendSms(String text){
  sim.sendMessage(text);  
}
 
void setup() {
  //Serial setup
  //Initiate serial with the computer
  Serial.begin(115200);
  //PIN setup
  pinMode(DOOR_SENSOR_P, INPUT_PULLUP);
  //Turn the sim module of on startup
  digitalWrite(7, LOW);
  //set the mode led on initally, because the modeule starts up with ALWAYS_SEND mode
  modeLed.ledOn();    
  Serial.println("ON");
}


void loop() {
  manualControl();
  sim.loop();
  modeButton.loop();
  modeLed.loop();
  if(!sending){
    //if we are not sending already, check for door trigger
    if(checkDoorOpen()){
      //Door opened
      if (timePassed(doorTriggerTime) > DELAY_FOR_DOOR_EVENTS){
        //Enough time has passed since the last send sequence
        Serial.println("Send sms");
        //store when the SMS sending was started to allow for delays between sends
        doorTriggerTime = millis();
        //Start send sequence
        startSendSequence();
      } else {
        Serial.println("Door delay active");
      }
    } 
  }
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
    if (digitalRead(DOOR_SENSOR_P)){
      //Door opened
      if (timePassed(doorMoveTime) > MINIMUM_SWITCH_TIME){
      //debounce time has passed
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

//calculates time passed from the time sent to the function
long timePassed(long time){
  return millis() + ((-1) * time);
}


//allows control of sim with serial port
void manualControl(){
  if(Serial.available()){
    Serial.println("man");
    while(Serial.available()){
      char pcChar = Serial.read();
      if (pcChar == INIT_CHAR){
        //0x1A is needed to send an SMS, Y used to be able to send it through terminal
        sim.setupSim();
      } else if (pcChar == OFF_CHAR) {
        Serial.println("off");
        sim.off();
      } else if (pcChar == TEST_MESSAGE_CHAR) {
        sim.sendMessage("Test");
      } else if (pcChar == SEND_SEQUENCE_CHAR) {
        startSendSequence();
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
          Serial.print("TIME FORMAT ERROR ");
          Serial.println(timeBufferCount);
        }
      } else if (pcChar == GET_TIME_CHAR){
        sim.getTime();
      } else if (pcChar == CHANGE_MODE_CHAR){
        switchSendMode();
      }
    }
  }
}

//Starts the send sequence which is: turn sim on, init, send, off
void startSendSequence(){
  //Initiate sim setup
  sim.setupSim();
  //The scetch will auomatically send the message when sim has been initialized
  sending = true;
}

/*
 * changes the mode of sending between send always, send depending on time, don't send
 * sets the status led as well
 */
void switchSendMode(){
  //switch mode of sending (1-3)
  sendMode++;
  switch(sendMode){
    case SEND_TIME:
      //led const on when sending depending on time
      modeLed.ledOn();
      break;
    case SEND_ALWAYS:
      //led blinking when sending always
      modeLed.ledBlink(500);
      break;
    case SEND_OFF:
      //led off when sending always
      modeLed.ledOff();
      break;
    default:
      //set to SEND_TIME, which is 1, so the ++ works
      sendMode = SEND_TIME;
      modeLed.ledOn();
      break;
  }
  Serial.print("Send mode: ");
  Serial.println(sendMode);
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
