#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "voltage.h"
#include "inttypes.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/adc_types_legacy.h"

#define TAG "VOLTAGE"

#define DEFAULT_VREF    1100        //Use adc2_vref_to_gpio() to obtain a better estimate
#define NO_OF_SAMPLES   64          //Multisampling

// ADC衰减设置 
// 0 dB 衰减(ADC_ATTEN_DB_0) 100mv ~ 950mv
// 2.5 dB 衰减(ADC_ATTEN_DB_2_5) 100mv ~ 1250mv
// 6 dB 衰减 (ADC_ATTEN_DB_6) 150mv ~ 1750mv
// 11 dB 衰减 (ADC_ATTEN_DB_11) 150mv ~ 2450mv
#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_12

static adc_oneshot_unit_handle_t adc1_handle;

static bool do_calibration[2][10];
adc_cali_handle_t adc1_cali_handle_chan[10];
adc_cali_handle_t adc2_cali_handle_chan[10];

static int adc_raw[2][10];
static int voltage[2][10];
static adc_channel_t EXAMPLE_ADC1_CHAN[MAX_VOLTAGE_SENSOR_ADC] = { EXAMPLE_ADC1_CHAN0, EXAMPLE_ADC1_CHAN1 };

typedef struct voltageMeterADCState_s {
    uint16_t voltageDisplayFiltered;         // battery voltage in 0.01V steps (filtered)
    uint16_t voltageUnfiltered;       // battery voltage in 0.01V steps (unfiltered)
    // pt1Filter_t displayFilter;
#if defined(USE_BATTERY_VOLTAGE_SAG_COMPENSATION)
    uint16_t voltageSagFiltered;      // battery voltage in 0.01V steps (filtered for vbat sag compensation)
    pt1Filter_t sagFilter;            // filter for vbat sag compensation
#endif
} voltageMeterADCState_t;

voltageMeterADCState_t voltageMeterADCStates[MAX_VOLTAGE_SENSOR_ADC];

static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t* out_handle);
static void example_adc_calibration_deinit(adc_cali_handle_t handle);

//Configure ADC
void voltageMeterADCInit() {
    //-------------ADC1 Init---------------//
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = EXAMPLE_ADC_ATTEN,
    };

    //-------------ADC1 Calibration Init---------------//
    for (int i = 0; i < MAX_VOLTAGE_SENSOR_ADC; i++) {
        ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, EXAMPLE_ADC1_CHAN[i], &config));
        do_calibration[ADC_UNIT_1][i] = example_adc_calibration_init(ADC_UNIT_1, EXAMPLE_ADC1_CHAN[i], EXAMPLE_ADC_ATTEN, &adc1_cali_handle_chan[i]);
    }
}

void voltageMeterADCDeinit() {
    //Tear Down
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    for (int_fast8_t i = 0; i < MAX_VOLTAGE_SENSOR_ADC; i++)
        if (do_calibration[ADC_UNIT_1][i]) {
            example_adc_calibration_deinit(adc1_cali_handle_chan[i]);
        }
}

void voltageMeterADCRefresh(void) {
    for (int i = 0; i < MAX_VOLTAGE_SENSOR_ADC; i++)
    {
        ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, EXAMPLE_ADC1_CHAN[i], &adc_raw[ADC_UNIT_1][i]));
        ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN[i], adc_raw[ADC_UNIT_1][i]);
        if (do_calibration[ADC_UNIT_1][i]) {
            ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle_chan[i], adc_raw[0][i], &voltage[ADC_UNIT_1][i]));
            ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN[i], voltage[ADC_UNIT_1][i]);
            voltageMeterADCStates[i].voltageUnfiltered = voltage[ADC_UNIT_1][i];
        }
        ESP_LOGI(TAG,"ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, EXAMPLE_ADC1_CHAN[i], voltage[ADC_UNIT_1][i]);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void voltageMeterADCRead(voltageSensorADC_e adcChannel, voltageMeter_t* voltageMeter) {

    voltageMeterADCState_t* state = &voltageMeterADCStates[adcChannel];

    voltageMeter->displayFiltered = state->voltageDisplayFiltered;
    voltageMeter->unfiltered = state->voltageUnfiltered;


}


/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t* out_handle)
{
    adc_cali_handle_t handle = NULL;
    esp_err_t ret = ESP_FAIL;
    bool calibrated = false;

#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Curve Fitting");
        adc_cali_curve_fitting_config_t cali_config = {
            .unit_id = unit,
            .chan = channel,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_curve_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

#if ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    if (!calibrated) {
        ESP_LOGI(TAG, "calibration scheme version is %s", "Line Fitting");
        adc_cali_line_fitting_config_t cali_config = {
            .unit_id = unit,
            .atten = atten,
            .bitwidth = ADC_BITWIDTH_DEFAULT,
        };
        ret = adc_cali_create_scheme_line_fitting(&cali_config, &handle);
        if (ret == ESP_OK) {
            calibrated = true;
        }
    }
#endif

    * out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    }
    else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    }
    else {
        ESP_LOGE(TAG, "Invalid arg or no memory");
    }

    return calibrated;
}

static void example_adc_calibration_deinit(adc_cali_handle_t handle)
{
#if ADC_CALI_SCHEME_CURVE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Curve Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_curve_fitting(handle));

#elif ADC_CALI_SCHEME_LINE_FITTING_SUPPORTED
    ESP_LOGI(TAG, "deregister %s calibration scheme", "Line Fitting");
    ESP_ERROR_CHECK(adc_cali_delete_scheme_line_fitting(handle));
#endif
}
