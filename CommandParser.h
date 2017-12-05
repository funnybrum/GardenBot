#ifndef _COMMANDPARSER_H_
#define _COMMANDPARSER_H_

#include "GardenBot.h"

class CommandParser {
private:
	void processCommand(char*);
public:
	void readCommand();
	void printTime(void);
};

extern CommandParser commandParser;

#endif
