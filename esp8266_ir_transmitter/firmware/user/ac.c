#include <esp8266.h>
#include <math.h>
#include "ac.h"
#include "driver/spi.h"
#include "ir.h"


static ACStatus status;
static ETSTimer updateTimer;

bool isPowered();

/**
 * Convert the AC settings to a binary representation
 */
uint8_t acEncode(const ACSettings settings, uint8_t * message, uint8_t length) {
    if (length < 12 * 4) {
    	os_printf("AC: FIXME: acEncode: message buffer too short (%d), expected at least 48.\n", length);
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
 * Send a command to the AC unit to power up
 */
void start(void) {
    if (!isPowered()) {
        status.settings.onOff = true;
        send(status.settings);
    }
}

/**
 * Send a command to the AC unit to power down
 */
void stop(void) {
    if (isPowered()) {
        status.settings.onOff = true;
        send(status.settings);
    }
}

void setTemperature(uint8_t temperature) {
    if (isPowered()) {
        status.settings.temp = temperature;
        send(status.settings);
    }
}


void setMode(ac_mode_t mode) {
    if (isPowered()) {
        status.settings.mode = mode;
        send(status.settings);
    }
}


void setFanSpeed(ac_fan_t fan) {
    if (isPowered()) {
        status.settings.fan = fan;
        send(status.settings);
    }
}


/**
 * Send a command to the AC unit to set the given parameters
 */
void send(ACSettings settings) {
    os_printf("AC: send\n");
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
 * Read temperature of a NTC thermistor via MCP3002
 *
 * @param channel - only value 0 or 1 is valid
 * @return measured temperature in degrees Celsius
 */
static float measureTemperature(uint8_t channel) {
    // value of R2 and R3 resistors
    // TODO allow for calibration
    static const int R2 = 10000; // channel0
    static const int R3 = 10000; // channel1
    // NTC R0 resistance value
    static const int NTC_R0 = 22000;
    // NTC B coefficient
    static const int NTC_B = 4150;
    // Power supply voltage
    static const float VCC = 3.3;

    // SPI command: start, sgl, channel0, msbf
    uint32_t adc = spi_transaction(HSPI, 4, 0b1101 | (1 << channel), 0, 0, 0, 0, 11, 0);
    os_printf("ADC value (ch %d): %d\n", channel, adc);
    float vout = adc * VCC / 1024;
    float temperature = ntc(NTC_B, NTC_R0, (VCC - vout) / vout * (channel == 0 ? R2 : R3));
    return temperature;
}

/**
 * Update status with current temperature
 */
void readTemperature(void) {
    // initialize SPI for communication with MCP3002
    // For some reason, if this is not initialized after IR command is sent, it does not work
    spi_init(HSPI);
    status.temperatureIn = measureTemperature(0);
    status.temperatureOut = measureTemperature(1);
    os_printf("AC: Temp in: %d, temp out: %d\n", (int) status.temperatureIn, (int) status.temperatureOut);
}

/**
 * Determine if the AC unit is powered up
 */
bool isPowered() {
    // TODO implement this
    os_printf("AC: isPowered: Not implemented yet\n");
    return true;
}


/**
 * Update temperature and power status
 */
void update(void *arg) {
    readTemperature();
    status.started = isPowered();
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

    update(NULL);

    // set up repeating task to update temperature
    os_timer_disarm(&updateTimer);
    os_timer_setfn(&updateTimer, (os_timer_func_t *)update, NULL);
    os_timer_arm(&updateTimer, 10000, true);
}
