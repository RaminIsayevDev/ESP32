#include "i2c_bus.h"
#include "esp_log.h"
#include "bme280.h"
#include "bme280_defs.h"
#include "bh1750.h"
#include "driver/i2c.h"
#include "esp_system.h"

#include "sensor_bme280_init.h"
#include "sensor_bme280_read_data.h"

#define I2C_MASTER_NUM 0  // Определение I2C_MASTER_NUM

static const char *TAG = "MAIN";

struct bme280_dev bme280_dev;
struct bme280_data comp_data;
bh1750_t bh1750_dev;

void app_main(void) {
    // 1. Инициализируем шину I2C
    if (i2c_master_init() == ESP_OK) {
        ESP_LOGI(TAG, "I2C bus initialized successfully");
    } else {
        ESP_LOGE(TAG, "Failed to initialize I2C bus");
        return;
    }

    // 2. Инициализируем BME280
    if (bme280_sensor_init() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize BME280");
        return;
    }

    // 3. Инициализируем BH1750
    if (bh1750_init(&bh1750_dev, I2C_MASTER_NUM) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize BH1750");
        return;
    }

    // 4. Основной цикл - читаем данные с обоих датчиков
    while(1) {
        // Читаем температуру, давление и влажность
        sensor_bme280_read_data(&bme280_dev, &comp_data);
        
        // Читаем уровень освещённости
        if (bh1750_read_light_level(&bh1750_dev, I2C_MASTER_NUM) == ESP_OK) {
            uint16_t light_level = bh1750_get_light_level(&bh1750_dev);
            ESP_LOGI(TAG, "Light level: %d lux", light_level);
        }
        
        vTaskDelay(pdMS_TO_TICKS(60000));
    }
}