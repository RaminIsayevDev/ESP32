#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/i2c.h"
#include "sensor_mq135_init.h"
#include "sensor_mq135.h"

static const char *TAG = "MQ135";

adc_oneshot_unit_handle_t adc1_handle = NULL;

esp_err_t sensor_mq135_init() {
    adc_oneshot_unit_init_cfg_t init_config1 = {
        .unit_id = MQ135_ADC_UNIT,
        .clk_src = ADC_RTC_CLK_SRC_DEFAULT
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));

    adc_oneshot_chan_cfg_t config = {
        .bitwidth = ADC_BITWIDTH_DEFAULT,
        .atten = MQ135_ATTEN
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, MQ135_ADC_CHANNEL, &config));

    ESP_LOGI(TAG, "MQ135 ADC initialized successfully");
    return ESP_OK;
}