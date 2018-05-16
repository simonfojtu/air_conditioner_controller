#ifndef __AC_H_
#define __AC_H_

#include "espmissingincludes.h"

typedef enum { AUTO, MAX, MED, MIN } ac_fan_t;
typedef enum { SUN, FAN, COOL, SMART, DROPS } ac_mode_t;

typedef struct {
    uint8_t temp;
    ac_fan_t fan;
    ac_mode_t mode;
    bool onOff;
    bool sleep;
} ACSettings;

typedef struct {
    ACSettings settings;
    float temperatureIn;
    float temperatureOut;
    bool started;
} ACStatus;

void start(void);
void stop(void);
void setTemperature(uint8_t temperatures);
void setMode(ac_mode_t mode);
void setFanSpeed(ac_fan_t fan);

void send(ACSettings settings);
ACStatus get(void);
void acInit(void);

#endif
