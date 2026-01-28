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
#include "esp_sleep.h"

struct bme280_dev bme280_dev;
bh1750_t bh1750_dev;


static const char *TAG = "SENSORS";

#define DEEP_SLEEP_TIME_SEC 900 // 15 minutes

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


static void sensors_task_one_shot(void *arg) {
    // Ждем подключения MQTT
    ESP_LOGI(TAG, "Waiting for MQTT connection...");
    if (!mqtt_app_wait_connected(30000)) {
        ESP_LOGE(TAG, "MQTT connection timeout. Going to sleep.");
        esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIME_SEC * 1000000ULL);
        esp_deep_sleep_start();
    }
    ESP_LOGI(TAG, "MQTT connected.");

    // Инициализация датчиков
    if (bme280_sensor_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize BME280");
    }

    if (bh1750_init(&bh1750_dev, I2C_MASTER_NUM) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize BH1750");
    }

    if (sensor_mq135_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize MQ135");
    }

    weather_data_t current_weather_data = {0};

    // Чтение BME280
    struct bme280_data bme_data = sensor_bme280_read_data(&bme280_dev);
    if (bme_data.temperature == 0 && bme_data.humidity == 0 && bme_data.pressure == 0) {
        ESP_LOGE(TAG, "Failed to read data from BME280 sensor");
    } else {
        current_weather_data.temp = bme_data.temperature;
        current_weather_data.humidity = bme_data.humidity;
        current_weather_data.pressure = bme_data.pressure;
    }
    
    // Чтение BH1750
    uint16_t light_level;
    if (bh1750_read_light_level(&bh1750_dev, I2C_MASTER_NUM, &light_level) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read data from BH1750 sensor");
        current_weather_data.lux = 0;
    } else {
        current_weather_data.lux = (float)light_level;
    }

    // Чтение MQ135
    float ppm = sensor_mq135_read();
    if (ppm == 0.0) {
        ESP_LOGW(TAG, "MQ135 sensor returned 0.0 ppm.");
    }
    current_weather_data.co2_ppm = (int)ppm;

    // Отправка данных
    char *json_output = format_sensor_data_as_json(&current_weather_data);
    if (json_output) {
        ESP_LOGI(TAG, "Sensor data JSON: %s", json_output);
        int msg_id = mqtt_app_publish_and_wait("home/weather", json_output, 1, 0, 5000);
        if (msg_id != -1) {
            ESP_LOGI(TAG, "Data published successfully, msg_id=%d", msg_id);
        } else {
            ESP_LOGE(TAG, "Failed to publish data");
        }
        free(json_output);
    }

    ESP_LOGI(TAG, "Entering deep sleep for %d seconds...", DEEP_SLEEP_TIME_SEC);
    // Настройка таймера пробуждения
    esp_sleep_enable_timer_wakeup(DEEP_SLEEP_TIME_SEC * 1000000ULL);
    esp_deep_sleep_start();
}

void sensors_init(void) {
    xTaskCreate(
        sensors_task_one_shot,
        "sensors_task_one_shot",
        4096,
        NULL,
        5,
        NULL
    );
}