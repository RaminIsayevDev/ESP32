#include <stdio.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "sensor_mq135.h"

static const char *TAG = "MQ135";

extern adc_oneshot_unit_handle_t adc1_handle;

float R0 = 76.63;  // Значение калибровки при 20C/33%RH (для воздуха)

// Функция для расчёта сопротивления датчика
static float get_rs(int adc_value) {
    // Преобразуем ADC значение в напряжение (0-3.3V для ESP32)
    float v_adc = ((float)adc_value / 4095.0) * 3.3;

    // Вычисляем сопротивление датчика Rs, предполагая что V_supply = 3.3V и нет внешнего делителя
    // Rs = RL * (V_supply - V_out) / V_out
    if (v_adc < 0.01) { // Избегаем деления на ноль или слишком малые значения
        return -1.0; // Возвращаем ошибку/невалидное значение
    }
    float rs = RL_VALUE * (3.3 - v_adc) / v_adc;
    return rs;
}

// Функция для расчёта концентрации CO2 (ppm)
static float calculate_ppm(float rs) {
    if (rs < 0) return 0.0; // Не считать, если rs невалидно
	float ratio = rs / R0;
	if (ratio <= 0) return 0.0; // Избегаем проблем с log/pow
	float ppm = PARA_A * pow(ratio, PARA_B);
	return ppm;
}

float sensor_mq135_read() {
    static bool is_warming_up = true;
    static int64_t start_time = 0;
    
    if (start_time == 0) {
        ESP_LOGI(TAG, "MQ135 sensor warming up for 90 seconds...");
        start_time = esp_timer_get_time();
    }

    if (is_warming_up) {
        if ((esp_timer_get_time() - start_time) < (90 * 1000 * 1000)) {
            return 0.0; // Возвращаем 0 во время прогрева
        } else {
            ESP_LOGI(TAG, "MQ135 sensor warm-up complete.");
            is_warming_up = false;
        }
    }

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
    
	// ESP_LOGI(TAG, "ADC: %d, RS: %.2f kOhm, PPM: %.2f", adc_reading, rs, ppm);
    
	return ppm;
}