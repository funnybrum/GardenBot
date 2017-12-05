#ifndef DEVICE_H_
#define DEVICE_H_

#include "GardenBot.h"

#define ZONES 4

class Device {
private:
	SHT1x *soilSensor[ZONES];
	Dht11 *airSensor;
public:
	void init(void);
	void getData(LogRecord*);
	void getSensors(LogRecord*);
	void getValves(LogRecord*);
	void setValve(int, bool);
	bool getValve(int);
	bool isValidHumidity(int8_t);
	void testValve(int zone, int duration);
};

extern Device device;

#endif /* SENSORS_H_ */
