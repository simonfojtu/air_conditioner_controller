#ifndef IR_h
#define IR_h

#include "espmissingincludes.h"

void irInit();
void processCommand(const uint16_t* parsedDurations, const uint8_t numParsedDurations);

#endif
