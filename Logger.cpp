#include "GardenBot.h"

#define AT24Cxx_CTRL_ID 0x50
#define MEM_SIZE (long)65536
#define RECORD_SIZE sizeof(LogRecord)
#define SETTINGS_SIZE sizeof(Settings)
#define RECORD_COUNT (MEM_SIZE) / RECORD_SIZE

void Logger::init() {
	readSettings();
}

Settings* Logger::getSettings() {
	return &settings;
}

void Logger::addLogRecord(LogRecord* record) {
	setLogRecordTime(record);

#ifdef DEBUG_MODE
	printLogRecord(record);
#endif

#ifndef DEBUG_MODE_DO_NOT_LOG
	EepromWrite(RECORD_SIZE * settings.next_record_index, (byte*) record);
	increaseRecordIndex();
	saveSettings();
#endif
}

void Logger::addLogMessage(char *message) {
	LogRecord record;
	memset(&record, 0, RECORD_SIZE);
	setLogRecordTime(&record);
	record.day += 100;
	memcpy(record.message.message, message, sizeof(record.message.message));

#ifdef DEBUG_MODE
	printLogRecord(&record);
#endif

#ifndef DEBUG_MODE_DO_NOT_LOG
	EepromWrite(RECORD_SIZE * settings.next_record_index, (byte*) &record);
	increaseRecordIndex();
	saveSettings();
#endif
}

void Logger::increaseRecordIndex() {
	settings.next_record_index++;
	if (settings.next_record_index >= RECORD_COUNT) {
		settings.next_record_index = 0;
	}
}

void Logger::setLogRecordTime(LogRecord *record) {
	getTime();
	record->minutes = RTC.minute;
	record->hour = RTC.hour;

	record->day = RTC.day;
	record->month = RTC.month;
	record->year = (uint8_t) (RTC.year - 2000);
}

void Logger::printLogRecord(LogRecord* record) {
	char buffer[64];
	sprintf(buffer, "[%02d.%02d.%4d %02d:%02d] ", record->day % 100,
			record->month, record->year + 2000, record->hour, record->minutes);

	Serial.print(buffer);

	if (record->day <= 31) {
		sprintf(buffer,
				"Temperature: %2dC, humidity: %2d%%, ",
				record->data.air_temperature,
				record->data.air_humidity);

		Serial.print(buffer);
#ifndef DEBUG_MODE
		Serial.println();
#endif

		for (int i = 0; i < ZONES; i++) {
#ifdef DEBUG_MODE
			if (!((record->data.valves >> i) & 1)) {
				Serial.print(F("off"));
			} else {
				Serial.print(F("on"));
			}
			if (i < ZONES - 1) {
				Serial.print(F(", "));
			} else {
    #ifdef DEBUG_MODE_FAKE_HUMIDITY_CONTROL
		        Serial.print(F(", z1_hum="));
		        Serial.print(record->data.zone[0].humidity);
    #endif
				Serial.println();
			}
#else
			if (record->data.zone[i].humidity > -5 ||
				record->data.zone[i].humidity < 105) {
				sprintf(buffer,
						"  * Zone %d: humidity=%3d%%, temperature=%2dC, ",
						i + 1,
						record->data.zone[i].humidity,
						record->data.zone[i].temperature);
				Serial.print(buffer);
			} else {
				Serial.print(F("  * Zone "));
				Serial.print(i + 1);
				Serial.println(F(": humidity=###%, temperature=###C, "));
			}

			if (!((record->data.valves>>i) & 1)) {
				Serial.print(F("not "));
			}
			Serial.println(F("irrigating"));
#endif
		}
	} else {
		char msg[sizeof(record->message.message) + 1];
		memcpy(msg, record->message.message, sizeof(record->message.message));
		msg[sizeof(record->message.message) + 1] = 0;
		Serial.println(msg);
	}
}

void Logger::printLogRecords() {
	long address = RECORD_SIZE * (settings.next_record_index);

	LogRecord record;
	LogRecord emptyRecord;
	memset(&emptyRecord, 0, RECORD_SIZE);

	// read all records, print the non-empty once
	for (int i = 0; i < RECORD_COUNT; i++) {
		EepromRead(address, (byte*) &record);

		// If we are after the last record index - check if we've found an
		// empty record. If so - skip to the start of the memory.
		if (address == settings.next_record_index * RECORD_SIZE) {
			if (memcmp(&record, &emptyRecord, RECORD_SIZE) == 0) {
				// empty record, skip to the start of records, minor
				// chance for error here, we may end with a record skipped
				i += (MEM_SIZE - address)/RECORD_SIZE - 1;
				address = 0;
				continue;
			}
		}

		printLogRecord(&record);

		address += RECORD_SIZE;
		if (address >= MEM_SIZE) {
			address = 0;
		}
	}
}

void Logger::reset() {
	byte buf[RECORD_SIZE];
	memset(buf, 0, sizeof(buf));

	Serial.println(F("Erasing memory..."));

#ifndef DEBUG_MODE_DO_NOT_LOG
	for (long address = 0; address < MEM_SIZE; address += sizeof(buf)) {
		EepromWrite(address, buf);
	}
#endif

	Serial.println(F("Memory erased."));

	settings.log_interval = 60 * 2;
	settings.next_record_index = 0;

	for (int i = 0; i < ZONES; i++) {
		settings.zone[i].start = (byte) 6;
		settings.zone[i].duration = (unsigned short) 30;
		settings.zone[i].interval = (byte) 3;
		// Set the time like it has just finished irrigating but alligned on
		// the closest round hour.
		settings.zone[i].last_irrigation = (eventManager.getCurrentTime() - 31)
				- ((eventManager.getCurrentTime() - 31) % 60);
		settings.zone[i].skip_irrigation_limit = (int8_t) 120;
		settings.zone[i].force_irrigation_limit = (int8_t) -40;
		settings.zone[i].force_irrigation_now = (byte) 0;
	}

	saveSettings();
	Serial.println(F("Default settings restored."));
}

void Logger::saveSettings() {
	for (int i = 0; i < ZONES; i++) {
		settings.zone[i].force_irrigation_now = 0;
	}
	settings.checksum = calculateChecksum();
	// Byte by byte as writing all data with one call results in failures
	for (int i = 0; i < SETTINGS_SIZE; i++) {
		RTC.setRAM(i, (byte*) &settings + i, 1);
	}
}

void Logger::readSettings() {
	for (int i = 0; i < SETTINGS_SIZE; i++) {
		RTC.getRAM(i, (byte*) &settings + i, 1);
	}
	if (settings.checksum != calculateChecksum()) {
		Serial.println(F("Settings checksum error, resetting!"));
		reset();
	}
}

// Warning - works only on single page
void Logger::EepromRead(unsigned int address, byte* data) {
	Wire.beginTransmission(AT24Cxx_CTRL_ID);
	Wire.write((int) (address >> 8));
	Wire.write((int) (address & 255));
	Wire.endTransmission();
	delay(5);
	Wire.requestFrom(AT24Cxx_CTRL_ID, RECORD_SIZE);
	Wire.readBytes(data, RECORD_SIZE);
}

// Warning - works only on single page
void Logger::EepromWrite(unsigned int address, byte* data) {
	Wire.beginTransmission(AT24Cxx_CTRL_ID);
	Wire.write((int) (address >> 8));
	Wire.write((int) (address & 255));
	Wire.write(data, RECORD_SIZE);
	Wire.endTransmission();
	delay(5);
}

unsigned char Logger::calculateChecksum() {
	unsigned char checksum = 0;
	unsigned char *addr = (unsigned char*)&settings;
	// To SETTINGS_SIZE - 1 in order to skip the checksum and the next record
	// count field.
	for (int i = 0; i < SETTINGS_SIZE - 5; i++) {
		checksum += *(addr + i);
	}
	return checksum;
}

Logger logger = Logger();
