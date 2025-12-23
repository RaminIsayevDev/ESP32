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
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"

struct bme280_data weather_data;
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

    weather_data = sensor_bme280_read_data(&bme280_dev);
    
}

void sensors_init(void) {
    xTaskCreate(
        sensors_task,
        "sensors_task",
    );
}