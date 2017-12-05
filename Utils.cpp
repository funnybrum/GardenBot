#include "GardenBot.h"

int getFreeRam () {
  extern int __heap_start, *__brkval;
  int v;
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval);
}

void configureBluetooth() {
	Serial.println(F("Configuring bluetooth..."));
	delay(1500);
	// Send commands and then wait for the module to parse them.The HC-06
	// doesn't parse \n\r, but waits for some timeout (1 sec) to parse the
	// command. In order for this to work the module should not be paird
	// (I think). The settings are preserved on power loss.
	// Baud rates:
	//	4 --------- 9600
	//	5 --------- 19200
	//	6 --------- 38400
    //	7 --------- 57600
    //	8 --------- 115200
	Serial.print(F("AT+NAMEGardenBot"));
	delay(1500);
	Serial.print(F("AT+PIN4688"));
	delay(1500);
	Serial.print(F("AT+BAUD4"));
	delay(1500);
	Serial.println(F("Bluetooth 'GardenBot' configured, pin=4688, baudrate=9600"));
	// Clear the AT commands output and flush the buffers
	Serial.flush();
	while (Serial.available() > 0) {
		Serial.read();
		delay(10);
	}
}

void getTime() {
	RTC.getTime();
	while (RTC.day > 31 || RTC.month > 12 || RTC.year > 2030 ||
		   RTC.hour > 23 || RTC.minute > 59 || RTC.second > 59) {
		delay(10);
		RTC.getTime();
	}
}

