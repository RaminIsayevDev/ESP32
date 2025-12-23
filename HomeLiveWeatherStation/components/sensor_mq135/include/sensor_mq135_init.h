#ifndef SENSOR_MQ135_INIT_H
#define SENSOR_MQ135_INIT_H

#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

extern adc_oneshot_unit_handle_t adc1_handle;

esp_err_t sensor_mq135_init();

#endif