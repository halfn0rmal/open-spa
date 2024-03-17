/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "soc/soc_caps.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "freertos/queue.h"
#include "inc/input_manager.h"

const static char *TAG = "EXAMPLE";

#define INPUT_TASK_STACK_SIZE    (2048)
#define INPUT_TASK_PRIORITY      (10)

/*---------------------------------------------------------------
        ADC General Macros
---------------------------------------------------------------*/


#define ADC_INPUT_1                 ADC_CHANNEL_0
#define ADC_INPUT_2                 ADC_CHANNEL_3
#define ADC_INPUT_3                 ADC_CHANNEL_6
#define ADC_INPUT_4                 ADC_CHANNEL_7

#define EXAMPLE_ADC_ATTEN           ADC_ATTEN_DB_11

static int adc_raw[4];
static int voltage[4];
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle);
static void example_adc_calibration_deinit(adc_cali_handle_t handle);
static QueueHandle_t input_state_queue = NULL;

bool set_state(int * voltage, int * raw)
{
    if(input_state_queue == NULL) {
        printf("Input state queue not initialized.\n");
        return false;
    }
    input_state_t state;
    memcpy(state.voltage, voltage, sizeof(int) * NUMBER_OF_INPUTS);
    memcpy(state.raw, raw, sizeof(int) * NUMBER_OF_INPUTS);
    
    return xQueueSend(input_state_queue, &state, 0);
}

bool get_state(input_state_t * state)
{
    if(xQueueReceive(input_state_queue, state, 0)) {
        return true;
    } else {
        return false;
    }
}

void read_adc(adc_oneshot_unit_handle_t adc1_handle, adc_cali_handle_t adc1_cali_handle, int channel, uint8_t inputNumber, int * voltage, int * raw)
{
    ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, channel, &raw[inputNumber]));
    //ESP_LOGI(TAG, "ADC%d Channel[%d] Raw Data: %d", ADC_UNIT_1 + 1, channel, adc_raw[inputNumber]);
    ESP_ERROR_CHECK(adc_cali_raw_to_voltage(adc1_cali_handle, adc_raw[inputNumber], &voltage[inputNumber]));
    //ESP_LOGI(TAG, "ADC%d Channel[%d] Cali Voltage: %d mV", ADC_UNIT_1 + 1, channel, voltage[inputNumber]);
}

void input_manager_task(void *pvParameters)
{
    // //-------------ADC1 Init---------------//
    adc_oneshot_unit_handle_t adc1_handle;
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = ADC_UNIT_1,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    // //-------------ADC1 Config---------------//
    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = EXAMPLE_ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_INPUT_1, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_INPUT_2, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_INPUT_3, &config));
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_INPUT_4, &config));

    //-------------ADC1 Calibration Init---------------//
    adc_cali_handle_t adc1_cali_chan0_handle = NULL;
    adc_cali_handle_t adc1_cali_chan1_handle = NULL;
    adc_cali_handle_t adc1_cali_chan2_handle = NULL;
    adc_cali_handle_t adc1_cali_chan3_handle = NULL;
    bool do_calibration1_chan0 = example_adc_calibration_init(ADC_UNIT_1, ADC_INPUT_1, EXAMPLE_ADC_ATTEN, &adc1_cali_chan0_handle);
    bool do_calibration1_chan1 = example_adc_calibration_init(ADC_UNIT_1, ADC_INPUT_2, EXAMPLE_ADC_ATTEN, &adc1_cali_chan1_handle);
    bool do_calibration1_chan2 = example_adc_calibration_init(ADC_UNIT_1, ADC_INPUT_3, EXAMPLE_ADC_ATTEN, &adc1_cali_chan2_handle);
    bool do_calibration1_chan3 = example_adc_calibration_init(ADC_UNIT_1, ADC_INPUT_4, EXAMPLE_ADC_ATTEN, &adc1_cali_chan3_handle);

    while (1) {
        read_adc(adc1_handle, adc1_cali_chan0_handle, ADC_INPUT_1, eInput1, voltage, adc_raw);
        read_adc(adc1_handle, adc1_cali_chan1_handle, ADC_INPUT_2, eInput2, voltage, adc_raw);
        read_adc(adc1_handle, adc1_cali_chan2_handle, ADC_INPUT_3, eInput3, voltage, adc_raw);
        read_adc(adc1_handle, adc1_cali_chan3_handle, ADC_INPUT_4, eInput4, voltage, adc_raw);
        set_state(voltage, adc_raw);
        vTaskDelay(pdMS_TO_TICKS(1000));        
    }

    //Tear Down
    ESP_ERROR_CHECK(adc_oneshot_del_unit(adc1_handle));
    if (do_calibration1_chan0) {
        example_adc_calibration_deinit(adc1_cali_chan0_handle);
    }
    if (do_calibration1_chan1) {
        example_adc_calibration_deinit(adc1_cali_chan1_handle);
    }
    if (do_calibration1_chan2) {
        example_adc_calibration_deinit(adc1_cali_chan2_handle);
    }
    if (do_calibration1_chan3) {
        example_adc_calibration_deinit(adc1_cali_chan3_handle);
    }
}


/*---------------------------------------------------------------
        ADC Calibration
---------------------------------------------------------------*/
static bool example_adc_calibration_init(adc_unit_t unit, adc_channel_t channel, adc_atten_t atten, adc_cali_handle_t *out_handle)
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

    *out_handle = handle;
    if (ret == ESP_OK) {
        ESP_LOGI(TAG, "Calibration Success");
    } else if (ret == ESP_ERR_NOT_SUPPORTED || !calibrated) {
        ESP_LOGW(TAG, "eFuse not burnt, skip software calibration");
    } else {
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

void init_input_task(void)
{
    input_state_queue = xQueueCreate(1, sizeof(input_state_t));
    xTaskCreate(input_manager_task, "input_manager_task", INPUT_TASK_STACK_SIZE, NULL, INPUT_TASK_PRIORITY, NULL);
}