#include "GardenBot.h"

#define MAX_COMMAND_SIZE 32

void CommandParser::processCommand(char* command) {
	Settings *settings = logger.getSettings();

	if (memcmp(command, "get logs", 8) == 0) {
		logger.printLogRecords();
	} else if (memcmp(command, "get time", 8) == 0) {
		printTime();
	} else if (memcmp(command, "get settings", 12) == 0) {
		Serial.print(F("Settings:\n  * Logging interval(min): "));
		Serial.println(settings->log_interval);

		for (int i = 0; i < ZONES; i++) {
			Serial.print(F("\n  * Zone "));
			Serial.print(i + 1);
			Serial.print(F(":\n   * Interval (days): "));
			Serial.println(settings->zone[i].interval);
			Serial.print(F("   * Start hour: "));
			Serial.println(settings->zone[i].start);
			Serial.print(F("   * Duration (min): "));
			Serial.println(settings->zone[i].duration);
			Serial.print(F("   * Hours since last irrigation: "));
			Serial.println((eventManager.getCurrentTime() -
							settings->zone[i].last_irrigation) / 60);
			Serial.print(F("  * Force irrigation limit: "));
			Serial.println(settings->zone[i].force_irrigation_limit);
			Serial.print(F("  * Skip irrigation limit: "));
			Serial.println(settings->zone[i].skip_irrigation_limit);
		}
	} else if (memcmp(command, "help", 4) == 0) {
		Serial.println(F("GardenBot command help:"));
		Serial.println(F("  Log data:"));
		Serial.println(F("    * 'get log' - get the logs"));
		Serial.println(F("  Setting the time:"));
		Serial.println(F("    * 'time sec [x]' - set the seconds to x"));
		Serial.println(F("    * 'time min [x]' - set the minutes to x"));
		Serial.println(F("    * 'time hour [x]' - set the hours to x"));
		Serial.println(F("    * 'time day [x]' - set the day of the month to x"));
		Serial.println(F("    * 'time month [x]' - set the month to x"));
		Serial.println(F("    * 'time year [x]' - set the year to x"));
		Serial.println(F("  Configuring the settings:"));
		Serial.println(F("    * 'get settings' - get the settings"));
		Serial.println(F("    * 'set log [x]' - set log interval to x minutes"));
		Serial.println(F("    * 'zone [x] interval [y]' - set zone x irrigation interval (days)"));
		Serial.println(F("    * 'zone [x] time [y]' - set zone x start time (hour of day)"));
		Serial.println(F("    * 'zone [x] duration [y]' - set zone x irrigation duration (minutes)"));
		Serial.println(F("    * 'zone [x] force [y]' - set zone x force irrigation humidity limit"));
		Serial.println(F("    * 'zone [x] skip [y]' - set zone x skip irrigation humidity limit)"));
		Serial.println(F("    * 'zone [x] start' - force zone x start irrigation now)"));
		Serial.println(F("    * 'zone [x] test [y]' - start zone x for y seconds)"));
		Serial.println(F("  Debug commands:"));
		Serial.println(F("    * 'get time' - get the current time"));
		Serial.println(F("    * 'get mem' - get the free memory"));
		Serial.println(F("    * 'get data' - get the current system state"));
		Serial.println(F("    * 'reset' - reset settings to default, erase logs"));
		Serial.println(F("    * 'bluetooth reset' - reset the bluetooth configuration"));
	} else if (memcmp(command, "reset", 5) == 0) {
		logger.reset();
	} else if (memcmp(command, "get data", 8) == 0) {
		LogRecord r = LogRecord();
		Serial.println(F("Sensor data:"));
		device.getData(&r);
		logger.setLogRecordTime(&r);
		logger.printLogRecord(&r);
	} else if (memcmp(command, "get mem", 6) == 0) {
		Serial.print(F("Free memory = "));
		Serial.println(getFreeRam());
	} else if (memcmp(command, "time ", 5) == 0) {
		RTC.stopClock();
		if (memcmp(command + 5, "min ", 4) == 0) {
			RTC.minute = atoi(command + 9);
		} else if (memcmp(command + 5, "hour ", 5) == 0) {
			RTC.hour = atoi(command + 10);
		} else if (memcmp(command + 5, "sec ", 4) == 0) {
			RTC.second = atoi(command + 9);
		} else if (memcmp(command + 5, "day ", 4) == 0) {
			RTC.day = atoi(command + 9);
		} else if (memcmp(command + 5, "month ", 6) == 0) {
			RTC.month = atoi(command + 11);
		} else if (memcmp(command + 5, "year ", 5) == 0) {
			RTC.year = atoi(command + 10);
		}
		RTC.setTime();
		RTC.startClock();
		// Correct the last log event timestamp.
		eventManager.init();
	} else if (memcmp(command, "set log ", 8) == 0) {
		settings->log_interval = atoi(command + 8);
		logger.saveSettings();
		eventManager.alignLogInterval();
	} else if (memcmp(command, "zone ", 5) == 0) {
		int zone = (*(command + 5)) - 48;
		if (zone < 1 || zone > ZONES) {
			Serial.println(F("Invalid zone index!"));
		} else {
			// 0 based indexing, correct the zone
			zone--;
			if (memcmp(command + 6, " interval ", 10) == 0) {
				settings->zone[zone].interval = atoi(command + 16);
			} else if (memcmp(command + 6, " time ", 6) == 0) {
				settings->zone[zone].start = atoi(command + 12);
			} else if (memcmp(command + 6, " duration ", 10) == 0) {
				settings->zone[zone].duration = atoi(command + 16);
			} else if (memcmp(command + 6, " force ", 7) == 0) {
				settings->zone[zone].force_irrigation_limit = atoi(command + 13);
			} else if (memcmp(command + 6, " skip ", 6) == 0) {
				settings->zone[zone].skip_irrigation_limit = atoi(command + 12);
			} else if (memcmp(command + 6, " start", 6) == 0) {
				settings->zone[zone].force_irrigation_now = 1;
			} else if (memcmp(command + 6, " test", 5) == 0) {
				Serial.print(F("Testing zone "));
				Serial.print(zone + 1);
				Serial.print(F(" for "));
				Serial.print(atoi(command + 11));
				Serial.println(F(" seconds"));
				device.testValve(zone, atoi(command + 11));
			} else {
				Serial.println(F("Invalid zone command!"));
			}
		}
		logger.saveSettings();
	} else if (memcmp(command, "bluetooth reset", 15) == 0) {
		configureBluetooth();
	}
	Serial.println(F("Done"));
}

void CommandParser::readCommand() {
	static char command[MAX_COMMAND_SIZE];
	static unsigned int pos = 0;

	while (Serial.available() > 0) {
		// Protection for command buffer overflow
		if (pos >= MAX_COMMAND_SIZE) {
			pos = 0;
		}
		byte nextChar = Serial.read();

		switch (nextChar) {
		case '\n':
		case '\r':
			if (pos == 0) {
				break;
			}
			command[pos++] = '\0';
			// process the command
			processCommand(command);
			pos = 0;
			break;
		default:
			command[pos++] = nextChar;
		}
	}
}

void CommandParser::printTime() {
	getTime();
	Serial.print(F("Time: "));
	Serial.print(RTC.hour);
	Serial.print(F(":"));
	Serial.print(RTC.minute);
	Serial.print(F(":"));
	Serial.print(RTC.second);
	Serial.print(F(" on "));
	Serial.print(RTC.day);
	Serial.print(F("."));
	Serial.print(RTC.month);
	Serial.print(F("."));
	Serial.println(RTC.year);
}

CommandParser commandParser = CommandParser();
