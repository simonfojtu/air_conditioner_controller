#include <esp8266.h>
#include <math.h>
#include "ac.h"
#include "driver/spi.h"
#include "ir.h"


static ACStatus status;
static ETSTimer updateTimer;


/**
 * Convert the AC settings to a binary representation
 */
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

/**
 * Convert the binary representation of the settings to the IR pulses lengths
 */
void irEncode(uint8_t* message, uint8_t message_length, uint16_t* encoded) {
    // encoded length is 2 * message_length + 3
    static const uint16_t SHORT = 11*50;
    static const uint16_t LONG = 32*50;
    // hardcoded header
    encoded[0] = 180*50;
    encoded[1] = 90*50;

    int index = 2;
    for (int i = 0; i < message_length; i++) {
        encoded[index++] = SHORT;
        encoded[index++] = (message[i] == 1 ? LONG : SHORT);
    }
    // Append a SHORT pulse at the end, as recorded
    encoded[index] = SHORT;
}

/**
 * Send a command to the AC unit to set the given parameters
 */
void set(ACSettings settings) {
    uint8_t encoded[48];
    uint16_t buffer[48*2+3];
    uint8_t ret = acEncode(settings, encoded, 48);
    if (ret != 0)
        return;

    status.settings = settings;
    irEncode(encoded, 48, buffer);
    processCommand(buffer, 48*2+3);

    // disable on/off and sleep toggle
    status.settings.onOff = false;
    status.settings.sleep = false;
}

/**
 * Get current status of the AC unit
 */
ACStatus get(void) {
    return status;
}


/**
 * Compute temperature in degrees C given NTC thermocouple parameters
 */
static float ICACHE_FLASH_ATTR ntc(int B, int R0, int R) {
    float rinf = R0 * exp(-B / (273.15 + 25));
    return B / log(R / rinf) - 273.15;
}


/**
 * Update status with current temperature
 */
void readTemperature(void) {
    // value of R2 and R3 resistors
    // TODO allow for calibration
    static const int R2 = 10000;
    static const int R3 = 10000;
    // NTC R0 resistance value
    static const int NTC_R0 = 22000;
    // NTC B coefficient
    static const int NTC_B = 4150;
    // Power supply voltage
    static const float VCC = 3.3;

    float vout;
    // SPI command: start, sgl, channel0, msbf
    vout = spi_transaction(HSPI, 4, 0b1101, 0, 0, 0, 0, 11, 0) * VCC / 1024;
    status.temperatureIn = ntc(NTC_B, NTC_R0, (VCC - vout) / vout * R2);

    vout = spi_transaction(HSPI, 4, 0b1111, 0, 0, 0, 0, 11, 0) * VCC / 1024;
    status.temperatureOut = ntc(NTC_B, NTC_R0, (VCC - vout) / vout * R3);
}

/**
 * Initialize subsystems and update timer
 */
void acInit(void) {
    // default values
    status.settings.temp = 21;
    status.settings.fan = AUTO;
    status.settings.mode = COOL;
    status.settings.onOff = false;
    status.settings.sleep = false;

    // initialize IR subsystem
    irInit();

    // initialize SPI for communication with MCP3002
    spi_init(HSPI);

    readTemperature();

    // set up repeating task to update temperature
    os_timer_disarm(&updateTimer);
    os_timer_setfn(&updateTimer, (os_timer_func_t *)readTemperature, NULL);
    os_timer_arm(&updateTimer, 60000, true);
}
