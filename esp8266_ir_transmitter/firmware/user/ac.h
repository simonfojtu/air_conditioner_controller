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


uint8_t acEncode(const ACSettings settings, uint8_t * message, uint8_t length);
void irEncode(uint8_t* message, uint8_t message_length, uint16_t* encoded);
void send(ACSettings settings);


#endif
