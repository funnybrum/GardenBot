#include "GardenBot.h"

void setup() {
	RTC.startClock();
	Serial.begin(9600);
	logger.init();
	eventManager.init();
	device.init();
	logger.addLogMessage("Ready...");
#ifdef DEBUG_MODE
	Serial.println(F("Debug mode!!!"));
	logger.reset();
#endif
	Serial.println(F("Ready..."));
}

void loop() {
	commandParser.readCommand();
	eventManager.processEvents();
}
