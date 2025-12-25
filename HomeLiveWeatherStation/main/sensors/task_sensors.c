#include "task_sensors.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bme280.h"
#include "bme280_defs.h"
#include "sensor_bme280_init.h"
#include "sensor_bme280_read_data.h"
#include "bh1750.h"
#include "sensor_mq135_init.h"
#include "sensor_mq135.h"
#include "i2c_bus.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

struct bme280_data weather_data;
uint16_t light_level;
float ppm;
struct bme280_dev bme280_dev;
bh1750_t bh1750_dev;
adc_oneshot_unit_handle_t adc1_handle;

static const char *TAG = "SENSORS";

static void sensors_task(void *arg) {
    // TODO Сделать саму задачу работы датчиков

    // Инициализация датчиков

    if (bme280_sensor_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize BME280");
        return;
    }

    if (bh1750_init(&bh1750_dev, I2C_MASTER_NUM) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize BH1750");
        return;
    }

    if (sensor_mq135_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MQ135");
        return;
    }

    // Получение данных

    while (1) {
        weather_data = sensor_bme280_read_data(&bme280_dev);
        light_level = bh1750_read_light_level(&bh1750_dev, I2C_MASTER_NUM);
        ppm = sensor_mq135_read();

        ESP_LOGI("SENSORS", "Measured: Temp=%.1f, Hum=%.1f, mmHg=%.1f, Lux=%.1f, ppm=%.1f", weather_data.temperature, weather_data.humidity, weather_data.pressure, (float)light_level, ppm);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
    
}

void sensors_init(void) {
    xTaskCreate(
        sensors_task,
        "sensors_task",
        4096,
        NULL,
        5,
        NULL
    );
}