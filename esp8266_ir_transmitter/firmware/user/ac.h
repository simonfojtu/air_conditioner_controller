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
} ACStatus;

void set(ACSettings settings);
ACStatus get(void);
void acInit(void);

#endif
