// Only modify this file to include
// - function definitions (prototypes)
// - include files
// - extern variable definitions
// In the appropriate section

#ifndef _GardenBot_H_
#define _GardenBot_H_
#include "Arduino.h"

#define ZONES 4

//add your includes for the project GardenBot here
#include "Debug.h"
#include "Dht11.h"
#include "SHT1x.h"
#include "DS1307new.h"
#include "Wire.h"
#include "Utils.h"
#include "Logger.h"
#include "Device.h"
#include "EventManager.h"
#include "CommandParser.h"

const byte VALVE_PIN[ZONES] = {5, 4, 3, 2};
const byte SOIL_SENSOR_DATA_PINS[ZONES] = {12, 11, 9, 8};
const byte SOIL_SENSOR_CLOCK_PINS[ZONES] = {10, 10, 10, 10};
const byte AIR_SENSOR_PIN = 6;

//end of add your includes here
#ifdef __cplusplus
extern "C" {
#endif
void loop();
void setup();
#ifdef __cplusplus
} // extern "C"
#endif

//add your function definitions for the project GardenBot here




//Do not add code below this line
#endif /* _GardenBot_H_ */
