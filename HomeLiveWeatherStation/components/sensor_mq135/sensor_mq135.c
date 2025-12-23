#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "sensor_mq135.h"

static const char *TAG = "MQ135";

extern adc_oneshot_unit_handle_t adc1_handle;

float R0 = 76.63;  // Значение калибровки при 20C/33%RH (для воздуха)

// Функция для расчёта сопротивления датчика (с учётом делителя напряжения)
static float get_rs(int adc_value) {
    // Преобразуем ADC значение в напряжение на входе делителя (0-3.3V для ESP32)
    float V_adc = (adc_value / 4095.0) * 3.3;
    
    // Вычисляем напряжение на выходе датчика (до делителя)
    // V_sensor = V_adc * (R_DIVIDER_SENSOR + R_DIVIDER_GND) / R_DIVIDER_GND
    float V_sensor = V_adc * (R_DIVIDER_SENSOR + R_DIVIDER_GND) / R_DIVIDER_GND;
    
    // Вычисляем сопротивление датчика по закону Ома
    // RS = RL * (V_supply - V_sensor) / V_sensor
    float RS = RL_VALUE * (V_SENSOR_MAX - V_sensor) / V_sensor;
    
    return RS;
}

// Функция для расчёта концентрации CO2 (ppm)
static float calculate_ppm(float rs) {
	float ratio = rs / R0;
	float ppm = PARA_A * pow(ratio, PARA_B);
	return ppm;
}

float sensor_mq135_read() {
	int adc_reading = 0;
    
	// Читаем значение с ADC
	esp_err_t ret = adc_oneshot_read(adc1_handle, MQ135_ADC_CHANNEL, &adc_reading);
	if (ret != ESP_OK) {
		ESP_LOGE(TAG, "Failed to read ADC: %s", esp_err_to_name(ret));
		return 0.0;
	}
    
	// Вычисляем сопротивление датчика
	float rs = get_rs(adc_reading);
    
	// Вычисляем концентрацию CO2
	float ppm = calculate_ppm(rs);
    
	ESP_LOGI(TAG, "ADC: %d, RS: %.2f kOhm, CO2: %.2f ppm", adc_reading, rs, ppm);
    
	return ppm;
    
}