# Door SMS notification

A device that sends an SMS notification when a door is opened. Setup on Atmega Atmel 328P arduino prototyping board together with SIM900 board. The schematic, part layout on the prototyping board and code included in this repository.

## Functions

* SMS notification of door opened event;
* 3 modes: send always, never send, send depending on time;
* Uses RTC on the SIM900 module to evaluate if SMS needs to be sent or not if sending depending on time enabled;
* Sends notification if battery gets low;
* Indication lights.

## Photos

