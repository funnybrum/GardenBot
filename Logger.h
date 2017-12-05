#ifndef _LOGGER_H_
#define _LOGGER_H_

#include "GardenBot.h"

struct ZoneData {
	int8_t humidity;
	int8_t temperature;
};

struct LogData {
	ZoneData zone[ZONES];

	int8_t air_temperature;
	int8_t air_humidity;

	byte valves;
};

struct LogMessage {
	char message[11];
};

struct LogRecord {
	byte year;
	byte month;
	// day encodes also the message type
	// day <= 31 -> log data
	// day >= 100 -> log message
	byte day;
	byte hour;
	byte minutes;

	union {
		LogMessage message;
		LogData data;
	};
};

struct ZoneConfig {
	// at what time to start (hour of day)
	// 1 byte, signed
	byte start;

	// duration is how many minutes to irrigate
	// 2 bytes, unsigned
	unsigned int duration;

	// interval is the day count between each irrigation
	// 1 byte, signed
	byte interval;

	// the time of the last irrigation (in minutes from 2014)
	// 4 bytes, unsigned
	unsigned long last_irrigation;

	// if humidity is > skip limit - skip irrigation
	// 1 byte, signed
	int8_t skip_irrigation_limit;

	// if humidity is < force limit - force irrigation
	// 1 byte, signed
	int8_t force_irrigation_limit;

	// force irrigation now, must not be saved
	// 1 byte
	byte force_irrigation_now;
};
// must be <= 56 bytes
// current size is 9 + 4 * 11 = 53 bytes
struct Settings {
	// logging interval, in minutes
	unsigned int log_interval;

	// irrigation zone settings
	ZoneConfig zone[ZONES];

	// index of the next log record
	unsigned int next_record_index;

	// checksum
	unsigned char checksum;
};

class Logger {
private:
	Settings settings;
	void EepromRead(unsigned int, byte*);
	void EepromWrite(unsigned int, byte*);
	void increaseRecordIndex(void);
public:
	void init();
	Settings* getSettings();
	void readSettings();
	void saveSettings();
	void reset();
	void printLogRecords();
	void printLogRecord(LogRecord*);
	void addLogRecord(LogRecord*);
	void addLogMessage(char*);
	void setLogRecordTime(LogRecord*);
	unsigned char calculateChecksum();
};

extern Logger logger;

#endif
