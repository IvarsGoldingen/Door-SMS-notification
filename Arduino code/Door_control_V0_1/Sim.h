#ifndef Sim_h
#define Sim_h

//This needs to be included in the library so it can access standard arduino functions
#include "Arduino.h"
#include <SoftwareSerial.h>

class Sim{
  SoftwareSerial  Sim900Serial;


  
  private:
    //private functions and variables
    //time between blinks when setting up
    const int SETTING_UP_LED_PERIOD = 500;
    //time between blinks when sim900 busy
    const int SIM_BUSY_LED_PERIOD = 100;
    //time between blinks when sim900 hada a functional error
    const int F_ERROR_LED_PERIOD = 100;
    //SIM900 chip baud
    const long SIM900_BAUD = 19200;
    //The amount of time in MS SIM900 is allowed to be setup before the Init loop exits with error
    const unsigned int SIM900_MAX_SETUP_TIME = 60000;
    //Time amount allowed for sending of SMS.Note that there is a defined delay when sending an sms: SMS_CZ_DELAY
    const unsigned int SMS_MAX_SEND_TIME = 30000;
    //TODO: move this to the constructor
    //SIM900 user specific data
    const String PIN = "2046";
    const String PHONE_NR = "+37128807288";
    //SIM900 possible mesages
    //MESSAGES FROM
    //Sim900 asking for pin
    const String ENTER_PIN_IN = "+CPIN: SIM PIN";
    //Sim900 ready to send SMS, etc.
    const String CALL_READY = "Call Ready";
    //MESSAGES TO
    //enter pin
    const String ENTER_PIN_OUT = "AT+CPIN=";
    //send sms
    const String SEND_SMS_OUT = "AT+CMGS=";
    //set time
    const String SET_TIME = "AT+CCLK=";
    //get time
    const String GET_TIME = "AT+CCLK?";
    //The amount of time in SMS sending it is waited before the ctrl+z character is sent
    const int SMS_CZ_DELAY = 15000;
    //Power on off pin
    const byte SIM_PWR_PIN = 7;
    //pause time when turning off hte sim pin using the PWR pin
    const int PWR_OFF_SWITCH_TIME = 2000;
    
    //variable to store the time when sending the SMS message was started
    long timeOfSMSSendStart = 0;
    //boolean for controlling when sim900 has finished a message
    bool rSimMsg = false;
    //Leds used for displaying the status of the SIM900 module
    byte statusLed;
    byte errorLed;
    //Variable to savi in when was the time of the last blink
    unsigned long timeOfLastStatusBlink = 0;
    //Used to track the time of when the ctrl+z char is sent during SMS send
    unsigned long timeOfCzDelay = 0;
    //SIM900 values
    char simOutBuf[32];
    byte simOutBufCount = 0;
    char simInBuffer[32];
    byte simInBufferCount = 0;
    //buffer used to store the text sent in an SMS
    char messageBuffer[64];
    byte messageLength;
    //Stores the time when the last power pin switch happened
    unsigned long pwrPinSwitchTime = 0;
    //Stores the step number of the power off
    byte pwrOffStep = 0;
    //Indicates if the initialization proces is going on
    bool initializing = false;
    //saves the time when setup was started to detect time out
    unsigned long startOfSetup = 0;
    
    //Function used to tell how much time in MS has passed since an event
    unsigned long timePassed(unsigned long time);
    //Displays the status of the SIM900 module using 2 leds
    void displayStatus();
    //Read char by char from sim900
    void simRead();
    //Write data received from sim900 to serial
    void wrSimToPc();
    //Check if the received message from SIM900 is an error
    bool errMsgFromSim();
    //Write simOutBuffer to sim900
    void wrToSim();
    //Read a finished message from SIM900 during initialization
    void initMsgR();
    //Clear simOutBuf and simInBuffer for next messages
    void clearBuffer();
    //Read a finished message from SIM900 during sending of sms
    void simSendingSMSMsgR();
    //Read a finished message from SIM900 during normal sim operation
    void simMsgR();
    //SMS sending step by step allowing parallell operation
    void sendSMS();
    //To turn off the sim: pwrPin LOW, 2s delay, pwrPin HIGH, 2s delay, pwrPin LOW
    void offSequence();
    //Setup sequence for SIM chip before communication can be started
    void setupSequence();
    //A pointer to a function in the main scetch. informs the main scetch of the status of SIM operations.
    void (*updateStatusCb)(byte) = 0;
    //A pointer to a function in the main scetch. Sends the time as a string.
    void (*sendTimeCb)(String) = 0;
    //a function for reading the time when sim module answers
    void simGettingTimeMsgR();
  public:
    //public classes and variables
    //constructor
    Sim(byte serPin1, byte serPin2, byte ledG, byte ledR, void(*cb)(byte), void(*cbTime)(String));

    //possible states of the SIM chip
    const byte ST_INIT = 0;
    const byte ST_OK = 1;
    const byte ST_INIT_ER = 2;
    const byte ST_SENDING = 3;
    const byte ST_OP_ER = 4;
    const byte ST_OFF = 5;
    const byte GETTING_TIME = 6;

    //possible status updates from Sim lib to main sketch
    static const byte INIT_FINISHED = 1;
    static const byte INIT_FAILED = 2;
    static const byte SMS_SENT = 3;
    static const byte SMS_SEND_FAILED = 4;
    static const byte TUREND_OFF = 5;
    
    //indicates the ready condition of the SIM900 chip
    byte simStatus = TUREND_OFF;//0 - initializing, 1 - ready, 2 - init error, 3 - sytem busy, 4 - operational error
    
    //setup function. Read messages from SIM900 inserts simcode if necessary
    void setupSim();
    //Send SMS function
    void sendMessage(String text);
    //Sim900 loop function
    void loop();
    //turn off sim c=hip
    void off();
    //TODO: make private
    //turn on sim c=hip
    void on();
    /*set real time clock time on the SIM900
    * text time should be like this: <yy/MM/dd,hh:mm:ss±zz> 
    * Example: <19/03/09,11:56:55+08>
    * The last 2 digits being time zone, see the sim module documentation
    */
    void setTime(String textTime);
    //TODO
    void getTime();

    /**Sending SMS values
    step 1: enter number
    step 2: enter content
    step 2.1: a long pause needed between 2 and 3
    step 3:end with 'ztrl+z' character ASCII number 26
    step 4: wait for success message
    **/
    //Allows start of SMS sending and executing the next sms step
    bool sendSms = false;
    //keeps track of the current step in the sending of the SMS
    byte smsSendStep = 0;
};
#endif
