#include "i2c_bus.h"
#include "esp_log.h"
#include "bme280.h"
#include "bme280_defs.h"
#include "driver/i2c.h"
#include "esp_system.h"

#include "sensor_bme280_init.h"
#include "sensor_bme280_read_data.h"

#define I2C_MASTER_NUM 0  // Добавлено определение I2C_MASTER_NUM

static const char *TAG = "MAIN";

struct bme280_dev bme280_dev;
struct bme280_data comp_data;

void app_main(void) {
    // 1. Инициализируем шину
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

    // 3. Тут можно добавить чтение данных или другие действия
    while(1) {
        sensor_bme280_read_data(&bme280_dev, &comp_data);

        bme280_dev.delay_us(1000000, bme280_dev.intf_ptr); // Пауза 1 секунда
    }
}