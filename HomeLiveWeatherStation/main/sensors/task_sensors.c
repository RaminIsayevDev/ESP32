#include "task_sensors.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "bme280.h"
#include "cJSON.h"
#include "data_structure.h"
#include "sensor_bme280_init.h"
#include "sensor_bme280_read_data.h"
#include "bh1750.h"
#include "sensor_mq135_init.h"
#include "sensor_mq135.h"
#include "i2c_bus.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "app_mqtt.h"

struct bme280_dev bme280_dev;
bh1750_t bh1750_dev;
adc_oneshot_unit_handle_t adc1_handle;

static const char *TAG = "SENSORS";

// Функция для форматирования данных сенсоров в JSON
static char* format_sensor_data_as_json(const weather_data_t *data) {
    cJSON *root = cJSON_CreateObject();
    if (root == NULL) {
        ESP_LOGE(TAG, "Failed to create cJSON object");
        return NULL;
    }

    cJSON_AddNumberToObject(root, "temperature", data->temp);
    cJSON_AddNumberToObject(root, "humidity", data->humidity);
    cJSON_AddNumberToObject(root, "pressure", data->pressure);
    cJSON_AddNumberToObject(root, "lux", data->lux);
    cJSON_AddNumberToObject(root, "co2_ppm", data->co2_ppm);

    char *json_string = cJSON_Print(root);
    if (json_string == NULL) {
        ESP_LOGE(TAG, "Failed to print cJSON object");
    }

    cJSON_Delete(root);
    return json_string;
}


static void sensors_task(void *arg) {
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

    weather_data_t current_weather_data;

    // Получение данных
    while (1) {
        struct bme280_data bme_data = sensor_bme280_read_data(&bme280_dev);
        if (bme_data.temperature == 0 && bme_data.humidity == 0 && bme_data.pressure == 0) {
            ESP_LOGE(TAG, "Failed to read data from BME280 sensor");
            current_weather_data.temp = 0;
            current_weather_data.humidity = 0;
            current_weather_data.pressure = 0;
        } else {
            current_weather_data.temp = bme_data.temperature;
            current_weather_data.humidity = bme_data.humidity;
            current_weather_data.pressure = bme_data.pressure;
        }
        
        uint16_t light_level;
        if (bh1750_read_light_level(&bh1750_dev, I2C_MASTER_NUM, &light_level) != ESP_OK) {
            ESP_LOGE(TAG, "Failed to read data from BH1750 sensor");
            current_weather_data.lux = 0;
        } else {
            current_weather_data.lux = (float)light_level;
        }

        float ppm = sensor_mq135_read();
        if (ppm == 0.0) {
            ESP_LOGW(TAG, "MQ135 sensor returned 0.0 ppm. It might be warming up or there is an issue.");
            current_weather_data.co2_ppm = 0;
        } else {
            current_weather_data.co2_ppm = (int)ppm;
        }

        char *json_output = format_sensor_data_as_json(&current_weather_data);
        if (json_output) {
            ESP_LOGI(TAG, "Sensor data JSON: %s", json_output);
            mqtt_app_publish("home/weather", json_output);
            free(json_output); // Важно освободить память после использования
        }


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
