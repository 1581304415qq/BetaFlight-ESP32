#pragma once
#include "soc/gpio_num.h"
#include "hal/adc_types.h"
#include "stdint.h"

#define MAX_VOLTAGE_SENSOR_ADC 2

#define EXAMPLE_ADC1_CHAN0 ADC_CHANNEL_0
#define EXAMPLE_ADC1_CHAN1 ADC_CHANNEL_1
#define EXAMPLE_ADC1_CHAN3 ADC_CHANNEL_3

#define ADC_UNIT ADC_UNIT_1

typedef enum {
    VOLTAGE_METER_NONE = 0,
    VOLTAGE_METER_ADC,
    VOLTAGE_METER_ESC,
    VOLTAGE_METER_COUNT
} voltageMeterSource_e;

typedef enum {
    VOLTAGE_SENSOR_ADC_VBAT = 0,
    VOLTAGE_SENSOR_ADC_12V = 1,
    VOLTAGE_SENSOR_ADC_9V = 2,
    VOLTAGE_SENSOR_ADC_5V = 3
} voltageSensorADC_e; // see also voltageMeterADCtoIDMap


typedef struct voltageMeter_s {
    uint16_t displayFiltered;               // voltage in 0.01V steps
    uint16_t prevDisplayFiltered;           // voltage in 0.01V steps
    uint32_t prevDisplayFilteredTime;
    uint16_t unfiltered;                    // voltage in 0.01V steps
    uint16_t sagFiltered;                   // voltage in 0.01V steps
    bool isVoltageStable;
    bool lowVoltageCutoff;
} voltageMeter_t;

void voltageMeterADCInit();
void voltageMeterADCDeinit();
void voltageMeterADCRefresh(void);
void voltageMeterADCRead(voltageSensorADC_e adcChannel, voltageMeter_t* voltageMeter);