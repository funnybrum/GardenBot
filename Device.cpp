#include "GardenBot.h"

/**
 * Note: Valve output pin control is inverser,
 * LOW turns on the valve, HIGH turns it off.
 */

void Device::init() {
	for (int i = 0; i < ZONES; i++) {
		// turn off the valve
		digitalWrite(VALVE_PIN[i], HIGH);
		pinMode(VALVE_PIN[i], OUTPUT);

		soilSensor[i] = new SHT1x(SOIL_SENSOR_DATA_PINS[i],
				SOIL_SENSOR_CLOCK_PINS[i]);
	}

	airSensor = new Dht11(AIR_SENSOR_PIN);
}

void Device::getData(LogRecord* record) {
	getSensors(record);
	getValves(record);
}

void Device::getSensors(LogRecord* record) {
	for (int i = 0; i < ZONES; i++) {
		float humidity;
		float temperature;
#ifdef DEBUG_MODE_DISABLE_HUMIDITY_CONTROL
		humidity = -1;
		temperature = -1;
#else
    #ifdef DEBUG_MODE_FAKE_HUMIDITY_CONTROL
		static int hc_current = 50;
		static int hc_step = 1;
		static int hc_target = 50;
		if (i == 0) {
			if (abs(hc_current - hc_target) < abs(hc_step)) {
				hc_target = millis() % 101;
				hc_step = 1 + (millis() % 5);
				if (hc_target < hc_current) {
					hc_step = -hc_step;
				}
			} else {
				hc_current += hc_step;
			}
			humidity = hc_current;
			temperature = 10;
		} else {
			humidity = -40;
			temperature = -40;
		}
    #else
		humidity = soilSensor[i]->readHumidity();
		temperature = soilSensor[i]->readTemperatureC();
    #endif
#endif
		record->data.zone[i].humidity = (int8_t) humidity;
		record->data.zone[i].temperature = (int8_t) temperature;
	}

	if (airSensor->read() == Dht11::OK) {
		record->data.air_humidity = (int8_t) airSensor->getHumidity();
		record->data.air_temperature = (int8_t) airSensor->getTemperature();
	} else {
		record->data.air_humidity = -100;
		record->data.air_temperature = -40;
	}
}

void Device::getValves(LogRecord* record) {
	record->data.valves = 0;
	for (int i = 0; i < ZONES; i++) {
		record->data.valves += getValve(i) ? (1 << i) : 0;
	}
}

void Device::setValve(int zone, bool state) {
	digitalWrite(VALVE_PIN[zone], state ? LOW : HIGH);
}

bool Device::getValve(int zone) {
	// HIGH -> off, LOW -> on, so inverse the result
	return !bitRead(PORTD, VALVE_PIN[zone]);
}

/**
 * Turn on the specified zone for the specfied seconds. After this the zone
 * returns to its prevous state.
 */
void Device::testValve(int zone, int duration) {
	bool state = getValve(zone);
	setValve(zone, true);
	delay(duration * 1000);
	setValve(zone, state);
}

bool Device::isValidHumidity(int8_t humidity) {
#ifdef DEBUG_MODE_DISABLE_HUMIDITY_CONTROL
	return false;
#else
	return humidity > -5 && humidity < 105;
#endif
}

Device device = Device();
