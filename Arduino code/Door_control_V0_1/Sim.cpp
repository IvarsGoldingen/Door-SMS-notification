//This needs to be included in the library so it can access standard arduino functions
#include "Arduino.h"
#include "Sim.h"
#include <SoftwareSerial.h>

//constructor for Sim object
Sim::Sim(byte serPin1, byte serPin2, byte ledG, byte ledR, void(*cb)(byte)):
//this syntax for SW serial allows the SW serial object be created with parameters
Sim900Serial(SoftwareSerial(serPin1, serPin2)){
  statusLed = ledG;
  errorLed = ledR;
  pinMode(statusLed, OUTPUT);
  pinMode(errorLed, OUTPUT);
  pinMode(SIM_PWR_PIN, OUTPUT);
  digitalWrite(SIM_PWR_PIN, LOW);
  updateStatusCb = cb;
}

void Sim::simRead(){
  //TODO: write timer that registeres unfinished messages after a certain time
  if(Sim900Serial.available()){
    simInBuffer[simInBufferCount++] = Sim900Serial.read();
    //check if message has ended with 0xD 0xA
    if(simInBuffer[simInBufferCount-1] == 0xA){
      if (simInBuffer[simInBufferCount-2] == 0xD){
        rSimMsg = true;
        if (simInBufferCount == 2){
          Serial.println("EMPT_M");
        }
      }
    }
  }
}

//writes SIM buffer to PC serial
void Sim::wrSimToPc(){
  if (simInBufferCount <= 2){
    //message is empty, do nothing
    //Serial.println("empty");
  } else {
    for(int i = 0; i < (simInBufferCount-2); i++){
      Serial.print(simInBuffer[i]);
    }
    Serial.println();
  }
}

bool Sim::errMsgFromSim(){
  //the SIM900 answers with ERROR if failed to send message or received an incorrect command
  //ERROR message is 5 letters + 2 unused symbols = 7 symbols
  if (simInBufferCount == 7){
    //the message is the same lengt as an error message
    if (simInBuffer[0] == 'E' &&
        simInBuffer[1] == 'R' &&
        simInBuffer[2] == 'R'){
      //an error message has been received
      //Set the status of the SIM to error
      simStatus = 2;
      //displayStatus();
      //return true when an error message has been received
      return true;        
    }
  }
  //The buffer does not contain the error message. Return false.
  return false;
}

//Function writes simOutBuf with the lenght of simOutBufCount  to SIM900
void Sim::wrToSim(){
  for(int i = 0; i < simOutBufCount; i++){
      Sim900Serial.print(simOutBuf[i]);
  }
  Sim900Serial.println();
  clearBuffer();
}


//Read messages from sim during initialization
//The only ones of interest are whem SIM900 asks for pin and when it is call ready
void Sim::initMsgR(){
  //check the message against possible variations
  //first check for equal message lengh. -2 because of end of message has 2 unused symbols
  //-1 because that a properly formatted string ends with the NULL symbol, which has ASCII value 0.
  if (ENTER_PIN_IN.length() == (simInBufferCount - 2)){
    Serial.println("ENTER_PIN");
    //SIM900 asking for PIN code, enter it
    //Combine the enter pin message with the pin code. Put quates around the PIN code
    String sumString = ENTER_PIN_OUT + PIN;
    sumString.toCharArray(simOutBuf, 32);
    Serial.println(simOutBuf);
    //-2 because each string has the NULL symbol at the end, but +2 because 2 quate marks have been added
    simOutBufCount = ENTER_PIN_OUT.length() + PIN.length();
    wrToSim();
  } else if (CALL_READY.length() == (simInBufferCount - 2)){
    //call ready message received. Initialization can now be exited
    Serial.println("SETUP SUCCESS");
    updateStatusCb(INIT_FINISHED);
    simStatus = ST_OK;
    displayStatus();
  }
}

void Sim::clearBuffer(){
  //TODO: making everything null pssibly useless?
  for(int j = 0; j < simOutBufCount; j++){
    simOutBuf[j] = NULL;
  }
  simOutBufCount = 0;
  for(int j = 0; j < simInBufferCount; j++){
    simInBuffer[j] = NULL;
  }
  simInBufferCount = 0;
}

/*control of status LEDs
0 - initializing, 1 - ready, 2 - init error, 3 - sytem busy, 4 - operational errorconst 
char ST_INIT = 0;
const char ST_OK = 1;
const char ST_INIT_ER = 2;
const char ST_BUSY = 3;
const char ST_OP_ER = 4;
*/
void Sim::displayStatus(){
  if (simStatus == ST_INIT){
    //Slow blink status led when initializing
    if (timePassed(timeOfLastStatusBlink) > SETTING_UP_LED_PERIOD){
      timeOfLastStatusBlink = millis();
      digitalWrite(statusLed, !digitalRead(statusLed));
    }
  } else if (simStatus == ST_OK){
    //Green led on when OK
    digitalWrite(statusLed, HIGH);
    digitalWrite(errorLed, LOW);
  } else if (simStatus == ST_INIT_ER){
    //Red led on when niitialization error
    digitalWrite(statusLed, LOW);
    digitalWrite(errorLed, HIGH);
  } else if (simStatus == ST_BUSY){
    //Blink status led when busy - sending sms or other
    if (timePassed(timeOfLastStatusBlink) > SIM_BUSY_LED_PERIOD){
      timeOfLastStatusBlink = millis();
      digitalWrite(statusLed, !digitalRead(statusLed));
    }
  } else if (simStatus == ST_OP_ER){
    //blinking red led when functional fault
    if (timePassed(timeOfLastStatusBlink) > F_ERROR_LED_PERIOD){
      timeOfLastStatusBlink = millis();
      digitalWrite(errorLed, !digitalRead(errorLed));
    }
  } else if (simStatus == ST_OFF){
    //Both leds off when SIM900 not operating
    digitalWrite(statusLed, LOW);
    digitalWrite(errorLed, LOW);
  }
}

void Sim::simSendingSMSMsgR(){
  //check the message comming from SIM900 against possible variations
  if (smsSendStep == 2){
    Serial.println(SEND_SMS_OUT.length() + PHONE_NR.length());
    Serial.println(simInBufferCount);
    Serial.println(simInBuffer);
  }
  
  if (smsSendStep == 1 && (simInBufferCount - 2) == 2){
    //Check if sim answers with OK
    Serial.println("CMGF=1 OK");
    sendSms = true;
  } else if (smsSendStep == 2  && ((SEND_SMS_OUT.length() + (PHONE_NR.length())) == (simInBufferCount - 4))){
    //-4 because 2 quates added around the number and 2 end of lince characters at end of message
    Serial.println("SMS_SEND_ECHO");
    //SIM900 echoing SMS message send, allow for the next step
    sendSms = true;
  } else if (smsSendStep == 3 && simInBuffer[0] == '>'){
    Serial.println("MESSAGE_ECHO");
    //in the second SMS send step SIM900 echos the message with > in the begining
    //allow for the next step
    sendSms = true;
    //the CZ send requires a delay
    timeOfCzDelay = millis();
  } else if (smsSendStep == 4 &&
             simInBuffer[0] == '+' &&
             simInBuffer[1] == 'C' &&
             simInBuffer[2] == 'M'){
    Serial.println("SMS_SUCCESS");     
    updateStatusCb(INIT_FINISHED);    
    //on succesful send SMS, the SIM900 answers with +CMGS: 64, where the number changes
    simStatus = SMS_SENT;
    smsSendStep = 0;
  }
}

//Read messages from sim during normal operation
void Sim::simMsgR(){
  //first check if the received message is an error message
  if (errMsgFromSim()){
    Serial.println("ERR_MSG_R");
    //Set the status of the SIM to error
    simStatus = ST_OP_ER;
    displayStatus();
    smsSendStep = 0;
  } else {
    //NOT an error message from SIM900
    //check the message comming from SIM900 against possible variations
    if (smsSendStep > 0){
      //if smsSending is initiated listen for thosse specific messages
      simSendingSMSMsgR();
    }
  }
}

void Sim::sendSMS(){
  if (smsSendStep == 1){
    Serial.println("STEP 1");
    //the first step is to initiate the mesage sending with the phone number
    //Combine the send SMS message and phone number. Put quates around the number
    String sumString = SEND_SMS_OUT + "\"" + PHONE_NR + "\"";
    //TODO not getting thsecond quote marks
    sumString.toCharArray(simOutBuf, 32);
    //+2 because 2 quate marks have been added
    simOutBufCount = SEND_SMS_OUT.length() + PHONE_NR.length() + 2;
    wrToSim();
    smsSendStep = 2;
  } else if (smsSendStep == 2){
    Serial.println("STEP 2");
    //Write the actaul SMS text to SIM900
    strcpy(simOutBuf, messageBuffer);
    simOutBufCount = messageLength;
    wrToSim();
    smsSendStep = 3;
  } else if (smsSendStep == 3){
    if (timePassed(timeOfCzDelay) > SMS_CZ_DELAY){
      //some time must pass before ctrl+z char can be sent to send the SMS
      Serial.println("STEP 3");
      //send the ztrl+z character to SIM900 for the message to be sent
      simOutBuf[0] = 0x1A;
      simOutBufCount = 1;
      wrToSim();
      //this will be set to 0 when success message is received
      smsSendStep = 4;
    } else {
      //If the time has not yet passed initiate the sendsSMS variable with true so the program enters the loop again
      sendSms = true;
    }
  }
}

unsigned long Sim::timePassed(unsigned long time){
  return millis() - time;
}

//turns on the sim chip
//the pin only needs to be pulled high
void Sim::on(){
  digitalWrite(SIM_PWR_PIN, HIGH);
  simStatus = ST_INIT;
}

//turns off the sim chip
//sequence on the pwr pin low 2s, high 2s, low
void Sim::off(){
  digitalWrite(SIM_PWR_PIN, LOW);
  pwrPinSwitchTime = millis();
  pwrOffStep = 1;
}

void Sim::offSequence(){
  if (timePassed(pwrPinSwitchTime) > PWR_OFF_SWITCH_TIME){
    if (pwrOffStep == 1){
      pwrOffStep = 2;
      pwrPinSwitchTime = millis();
      digitalWrite(SIM_PWR_PIN, HIGH);
    } else if (pwrOffStep == 2){
      pwrOffStep = 0;
      digitalWrite(SIM_PWR_PIN, LOW);
      simStatus = ST_OFF;
      updateStatusCb(TUREND_OFF);
    } 
  }
}

void Sim::sendMessage(String text){
  sendSms = true;
  smsSendStep = 1;
  text.toCharArray(messageBuffer, 64);
  messageLength = text.length();
  timeOfSMSSendStart = millis();
}

void Sim::setupSim(){
  Sim900Serial.begin(SIM900_BAUD);
  on();
  startOfSetup = millis();
  simStatus == ST_INIT;
}

//void Sim::setupSequence(){
//   //allow certain amount of time for setting up of SIM
//  while(simStatus == ST_INIT){
//    //Indicate initialization with blinking
//    displayStatus();
//    simRead();
//    if (rSimMsg){
//      //the message from sim has been finished
//      wrSimToPc();
//      
//      clearBuffer();
//      rSimMsg = false;
//    }
//    if (timePassed(startOfSetup) > SIM900_MAX_SETUP_TIME){
//      //Initialization timed out
//      Serial.println("INIT TO");
//      //Set the status of the SIM to error
//      simStatus = ST_INIT_ER;
//      displayStatus();
//      //Inform main sketch of error
//      updateStatusCb(INIT_FAILED);
//      break;
//    }
//  }
//  Serial.println("SIM setup finished");
//}

void Sim::loop(){
  displayStatus();
  simRead();
  if (rSimMsg){
    //the message from sim has been finished
    wrSimToPc();
    //determine which type of messages are to be read
    if (simStatus == ST_INIT){
      //TODO send status to main SKETCH in case of ERROR from SIM
      initMsgR();
    } else {
      simMsgR();
    }
    clearBuffer();
    rSimMsg = false;
  }
  if (simStatus = ST_INIT){
    if (timePassed(startOfSetup) > SIM900_MAX_SETUP_TIME){
      //Initialization timed out
      Serial.println("INIT TO");
      //Set the status of the SIM to error
      simStatus = ST_INIT_ER;
      updateStatusCb(INIT_FAILED);
    }
  } else if (sendSms){
    if (timePassed(timeOfSMSSendStart) > SMS_MAX_SEND_TIME){
      Serial.println("SMS_SEND_TO");
      smsSendStep = 0;
      sendSms = false;
      simStatus = ST_OP_ER;
      //Inform main sketch of error
      updateStatusCb(SMS_SEND_FAILED);
    }
    //Wait for answer before next step
    sendSms = false;
    sendSMS();
  } else if (pwrOffStep > 0){
    offSequence();
  }
}