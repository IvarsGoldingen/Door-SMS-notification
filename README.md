# Door SMS notification

A device that sends an SMS notification when a door is opened. Setup on Atmega Atmel 328P arduino prototyping board together with SIM900 board. The schematic, part layout on the prototyping board and code included in this repository.

## Functions

* SMS notification of door opened event;
* 3 modes: send always, never send, send depending on time;
* Uses RTC on the SIM900 module to evaluate if SMS needs to be sent or not if sending depending on time enabled;
* Sends notification if battery gets low;
* Indication lights.

## Photos

![IMG_20190408_070154](https://user-images.githubusercontent.com/25284066/55977269-1134c980-5c97-11e9-98c1-4ae78c5deb4d.jpg)
