#ifndef IR_h
#define IR_h

#include "espmissingincludes.h"

void irInit();
//void setupReceive();
//
//extern bool isRecording;
//extern bool hasRecordedData;
//extern bool recordingOverflowed;
//
//void startRecording();
//void stopRecording();
//void processRecordedData();

void processCommand(const uint16_t* parsedDurations, const uint8_t numParsedDurations);

#endif
