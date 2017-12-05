#include "GardenBot.h"

void(* reset) (void) = 0; //declare reset function @ address 0

static const int DAYS_IN_MONTH[] = {-1, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};

unsigned long EventManager::getTodayStartTime() {
	unsigned long result = 0;

	getTime();

	for (int i = 2014; i < RTC.year; i++) {
		if (i%4) {
			result += 365;
		} else {
			result += 366;
		}
	}

	for (int i = 1; i < RTC.month; i++) {
		result += DAYS_IN_MONTH[i];
		if (i == 2 && RTC.year % 4 == 0) {
			result += 1;
		}
	}

	result += RTC.day;
	result *= 24;

	return result * 60;
}

// Gets the RTC time and based on it calculate the time from start
// of the day in minutes.
unsigned long EventManager::getTodayTime() {
	unsigned long result = 0;

	getTime();

	result += RTC.hour * 60;
	result += RTC.minute;
	return result;
}

// Gets the RTC time and based on it calculate the current time in minutes
// from the start of year 2014. Minute based resolution provides 45 years
// before the clock rolls over in 4 bytes.
unsigned long EventManager::getCurrentTime() {
#ifndef DEBUG_MODE
	return getTodayStartTime() + getTodayTime();
#else
	//Fix an issue with the faster software debug clock
	static unsigned int max = 0;
	unsigned long c = getTodayStartTime() + getTodayTime();

	while (c < max) {
		c = getTodayStartTime() + getTodayTime();
	}

	max = c;
	return c;
#endif
}

void EventManager::init() {
	// Some invalid value to force sensor reading on first pass.
	currentHour = 100;
	alignLogInterval();
}

void EventManager::alignLogInterval() {
	Settings *settings = logger.getSettings();
	// Force ASAP log event aligned at 00:00 .
	unsigned long now = getCurrentTime();
	lastLogEvent = getTodayStartTime() - settings->log_interval;
	while (lastLogEvent + settings->log_interval < now) {
		lastLogEvent += settings->log_interval;
	}
}

/**
 * Check if should start/stop irrigating. Resume irrigation on restart.
 * Returns true iff a valve state has changed.
 */
bool EventManager::checkZone(ZoneConfig *zone, ZoneData *data, int index, bool changeInHour) {
	int hour = RTC.hour;

	// Failsafe check - if zone last irrigation time is in futer - ignore the
	// zone till that moment as it would drown.
	if (zone->last_irrigation > currentTime) {
		return false;
	}

	// Check if the zone should be irrigating right now.
	if (zone->last_irrigation + zone->duration >= currentTime &&
		device.getValve(index) == false) {
		zone->force_irrigation_now = 0;
#ifdef DEBUG_MODE1
		Serial.print(currentTime);
		Serial.print(F(" Restart irrigating zone "));
		Serial.println(index + 1);
#endif
		// Generally this should not happen, but this serves to restart
		// irrigation on power loss.
		device.setValve(index, true);
		return true;
	}

	// Force irrigation flag.
	if (zone->force_irrigation_now) {
		zone->force_irrigation_now = 0;
		// Consider the flag only if the zone is currently not irrigating,
		// ignore it otherwise.
		if (device.getValve(index) == false) {
			zone->last_irrigation = currentTime;
			device.setValve(index, true);
			return true;
		}
		return false;
	}

	// Check if not irrigating. If so - check if should start irrigating
	// because of the irrigation interval or the low humidity limit.
	if (device.getValve(index) == false) {
		// Do the start irrigation check only once each hour!
		if (!changeInHour) {
			return false;
		}
		// Check the start irrigation hour
		if (zone->start == hour) {
			// Interval based irrigation.
			unsigned long interval = zone->interval * 24L * 60L;
			if (zone->last_irrigation + interval <= currentTime) {
				// Validate humidity, if it is too high - don't irrigate.
				if (device.isValidHumidity(data->humidity) &&
					data->humidity >= zone->skip_irrigation_limit) {
					return false;
				}

#ifdef DEBUG_MODE1
				Serial.print(currentTime);
				Serial.print(F(" Start irrigating zone "));
				Serial.println(index + 1);
#endif
				zone->last_irrigation = currentTime;
				device.setValve(index, true);
				return true;
			}

			// Low humidity based irrigation.
			if (device.isValidHumidity(data->humidity) &&
				data->humidity <= zone->force_irrigation_limit) {
#ifdef DEBUG_MODE
				Serial.print(currentTime);
				Serial.print(F(" Force irrigating zone "));
				Serial.print(index + 1);
				Serial.print(F(", humidity is "));
				Serial.println(data->humidity);
#endif
				// Last check is used to avoid multiple irrigation in the same
				// time, i.e. 10 min duration would result in 6 irrigations durring
				// 'currentTime' hour.
				zone->last_irrigation = currentTime;
				device.setValve(index, true);
				return true;
			}
		}
	} else {
		// Check if should stop irrigating.
		if (zone->last_irrigation + zone->duration < currentTime) {
#ifdef DEBUG_MODE1
			Serial.print(currentTime);
			Serial.print(F(" Stop irrigating zone "));
			Serial.println(index + 1);
#endif
			device.setValve(index, false);
			return true;
		}
	}

	return false;
}

void EventManager::processEvents() {
	currentTime = getCurrentTime();
	Settings *settings = logger.getSettings();
	bool forceLogEvent = false;
	bool changeInHour = false;

	// Security check to validate that ther are no bad values in the memory
	validateControllerState();

	// Every hour update the sensor readings.
	if (currentHour != RTC.hour) {
		currentHour = RTC.hour;
		changeInHour = true;
		memset(&currentData, 0, sizeof(LogRecord));
		device.getSensors(&currentData);
	}

	for (int i = 0; i < ZONES; i++) {
		bool zoneForceLogEvent = checkZone(&settings->zone[i],
				                           &currentData.data.zone[i],
				                           i,
				                           changeInHour);
		forceLogEvent = forceLogEvent || zoneForceLogEvent;
	}

	// Always get valves state after zone processing as some valves may change
	// state in that processing and we need that info up to date on the next
	// pass.
	device.getValves(&currentData);

	if (lastLogEvent + settings->log_interval <= currentTime) {
		lastLogEvent += settings->log_interval;
		logger.addLogRecord(&currentData);
	} else if (forceLogEvent) {
		// For forced events update sensor data as it may be old.
		device.getSensors(&currentData);
		logger.addLogRecord(&currentData);
	}
}

void EventManager::validateControllerState() {
	bool failure = false;

	if (logger.getSettings()->checksum != logger.calculateChecksum()) {
		Serial.println(F("Detected settings error, reseting"));
		logger.reset();
		reset();
	}
}

EventManager eventManager = EventManager();
