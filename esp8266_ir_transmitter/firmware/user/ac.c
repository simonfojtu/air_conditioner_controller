#include <esp8266.h>
#include "ac.h"
#include "ir.h"

uint8_t acEncode(const ACSettings settings, uint8_t * message, uint8_t length) {
    if (length < 12 * 4) {
    	os_printf("FIXME: acEncode: message buffer too short (%d), expected at least 48.\n", length);
        return -1;
    }

    //                   <-           HEADER             ->
    uint8_t header[] = {1,1,0,0, 0,0,0,1, 0,1,1,0, 0,0,0,0};
    memcpy(message, header, 16);

    message[18] = settings.onOff ? 1 : 0;
    message[19] = settings.sleep ? 1 : 0;
    // fan
    switch (settings.fan) {
        case (AUTO):
            message[16] = 0;
            message[17] = 0;
            break;
        case (MAX):
            message[16] = 1;
            message[17] = 0;
            break;
        case (MED):
            message[16] = 0;
            message[17] = 1;
            break;
        case (MIN):
            message[16] = 1;
            message[17] = 1;
            break;
    }
    // mode
    switch (settings.mode) {
        case (SUN):
            message[24] = 0;
            message[25] = 0;
            message[26] = 0;
            break;
        case (FAN):
            message[24] = 0;
            message[25] = 0;
            message[26] = 1;
            break;
        case (COOL):
            message[24] = 0;
            message[25] = 1;
            message[26] = 0;
            break;
        case (SMART):
            message[24] = 1;
            message[25] = 0;
            message[26] = 0;
            break;
        case (DROPS):
            message[24] = 1;
            message[25] = 1;
            message[26] = 0;
            break;
    }
    // temperature
    {
        uint8_t t = settings.temp - 16;
        message[28] = t & 0x1;
        message[29] = (t & 0x2) >> 1;
        message[30] = (t & 0x4) >> 2;
        message[31] = (t & 0x8) >> 3;
    }

    return 0;
}

void irEncode(uint8_t* message, uint8_t message_length, uint16_t* encoded) {
    // encoded length is 2 * message_length + 3
    static const uint16_t S = 11*50;
    static const uint16_t L = 32*50;
    // hardcoded header
    encoded[0] = 180*50;
    encoded[1] = 90*50;

    int index = 2;
    for (int i = 0; i < message_length; i++) {
        encoded[index++] = S;
        encoded[index++] = (message[i] == 1 ? L : S);
    }
    // Append a SHORT pulse at the end, as recorded
    encoded[index] = S;
}

void send(ACSettings settings) {
    uint8_t encoded[48];
    uint16_t buffer[48*2+3];
    uint8_t ret = acEncode(settings, encoded, 48);
    if (ret != 0)
        return;

    irEncode(encoded, 48, buffer);
    processCommand(buffer, 48*2+3);
}



