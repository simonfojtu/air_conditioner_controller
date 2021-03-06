#include "ir.h"

#include <i2s_reg.h>
#include <gpio.h>
#include <osapi.h>

#define TIMER_INTERVAL_US   (62)
#define IR_OUTPUT_PIN       (14)

#define MAX_DURATIONS       (1024)

uint16_t numParsedDurations = 0;
uint16_t parsedDurations[MAX_DURATIONS];

void startCarrier()
{
    WRITE_PERI_REG(PERIPHS_IO_MUX_MTMS_U,
        (READ_PERI_REG(PERIPHS_IO_MUX_MTMS_U)
        & 0xfffffe0f)
        | (0x1 << 4)
    );

    WRITE_PERI_REG(I2SCONF,
        (READ_PERI_REG(I2SCONF)
        & 0xf0000fff) | (                                                 // Clear I2SRXFIFO, BCK_DIV and CLKM_DIV sections
            ((TIMER_INTERVAL_US & I2S_BCK_DIV_NUM) << I2S_BCK_DIV_NUM_S) // Set the clock frequency divider
            | ((2 & I2S_CLKM_DIV_NUM) << I2S_CLKM_DIV_NUM_S)             // Set the clock prescaler
            | ((1 & I2S_BITS_MOD) << I2S_BITS_MOD_S)                     // ?
        )
    );

    WRITE_PERI_REG(I2SCONF,
        READ_PERI_REG(I2SCONF) | I2S_I2S_RX_START                       // Set the I2S_I2S_RX_START bit
    );
}

void stopCarrier()
{
    WRITE_PERI_REG(I2SCONF,
        READ_PERI_REG(I2SCONF) & 0xfffffdff                             // Clear I2S_I2S_RX_START bit
    );

    PIN_FUNC_SELECT(PERIPHS_IO_MUX_MTMS_U, FUNC_GPIO14);                // Set the MTMS pin function to standard GPIO
    GPIO_OUTPUT_SET(GPIO_ID_PIN(IR_OUTPUT_PIN), 0);                     // Clear the output
}

void irInit()
{
    stopCarrier();
}

void processCommand(const uint16_t* parsedDurations, const uint8_t numParsedDurations)
{
    for (int index = 0; index < numParsedDurations; index += 2)
    {
        startCarrier();
        os_delay_us(parsedDurations[index]);
        
        stopCarrier();
        if (index + 1 < numParsedDurations)
            os_delay_us(parsedDurations[index + 1]);
    }
}
