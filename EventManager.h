#ifndef EVENTS_H_
#define EVENTS_H_

#include "GardenBot.h"

class EventManager {
private:
	unsigned long lastLogEvent;
	unsigned long currentTime;
	byte currentHour;
	LogRecord currentData;
	bool checkZone(ZoneConfig *, ZoneData *, int, bool);
	void validateControllerState();
public:
	void init();
	void alignLogInterval();
	void processEvents();
	unsigned long getCurrentTime();
	unsigned long getTodayStartTime();
	unsigned long getTodayTime();
};

extern EventManager eventManager;

#endif /* EVENTS_H_ */
