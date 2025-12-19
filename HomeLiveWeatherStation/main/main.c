#include "i2c_bus.h"
#include "esp_log.h"
#include "bme280.h"
#include "bme280_defs.h"
#include "driver/i2c.h"
#include "esp_system.h"

#define I2C_MASTER_NUM 0  // Добавлено определение I2C_MASTER_NUM

static const char *TAG = "MAIN";

// Глобальная структура для BME280 (можно сделать статической)
static struct bme280_dev bme280_dev;

// Функции для чтения/записи по I2C (требуются драйвером BME280)
static int8_t bme280_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BME280_I2C_ADDR_PRIM << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BME280_I2C_ADDR_PRIM << 1) | I2C_MASTER_READ, true);
    if (len == 1) {
        i2c_master_read_byte(cmd, reg_data, I2C_MASTER_NACK);
    } else {
        for (uint32_t i = 0; i < len - 1; i++) {
            i2c_master_read_byte(cmd, &reg_data[i], I2C_MASTER_ACK);
        }
        i2c_master_read_byte(cmd, &reg_data[len - 1], I2C_MASTER_NACK);
    }
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return (ret == ESP_OK) ? 0 : -1;
}

static int8_t bme280_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (BME280_I2C_ADDR_PRIM << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd, reg_addr, true);
    i2c_master_write(cmd, reg_data, len, true);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(I2C_MASTER_NUM, cmd, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd);
    return (ret == ESP_OK) ? 0 : -1;
}

static void bme280_delay_us(uint32_t period, void *intf_ptr) {
    ets_delay_us(period);
}

// Ваша функция инициализации BME280
esp_err_t bme280_sensor_init(void) {
    // Настройка структуры устройства
    bme280_dev.intf = BME280_I2C_INTF;  // I2C интерфейс
    bme280_dev.intf_ptr = NULL;  // Не нужен для I2C
    bme280_dev.read = bme280_i2c_read;
    bme280_dev.write = bme280_i2c_write;
    bme280_dev.delay_us = bme280_delay_us;

    // Инициализация датчика
    int8_t rslt = bme280_init(&bme280_dev);
    if (rslt != BME280_OK) {
        ESP_LOGE(TAG, "BME280 init failed: %d", rslt);
        return ESP_FAIL;
    }

    // Настройка параметров (опционально, пример для нормального режима)
    struct bme280_settings settings;
    settings.osr_h = BME280_OVERSAMPLING_1X;  // Влажность
    settings.osr_p = BME280_OVERSAMPLING_1X;  // Давление
    settings.osr_t = BME280_OVERSAMPLING_1X;  // Температура
    settings.filter = BME280_FILTER_COEFF_OFF;  // Фильтр
    settings.standby_time = BME280_STANDBY_TIME_1000_MS;  // Время ожидания

    rslt = bme280_set_sensor_settings(BME280_SEL_OSR_PRESS | BME280_SEL_OSR_TEMP | BME280_SEL_OSR_HUM | BME280_SEL_FILTER | BME280_SEL_STANDBY, &settings, &bme280_dev);
    if (rslt != BME280_OK) {
        ESP_LOGE(TAG, "BME280 settings failed: %d", rslt);
        return ESP_FAIL;
    }

    // Перевод в нормальный режим
    rslt = bme280_set_sensor_mode(BME280_POWERMODE_NORMAL, &bme280_dev);
    if (rslt != BME280_OK) {
        ESP_LOGE(TAG, "BME280 mode set failed: %d", rslt);
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "BME280 initialized successfully");
    return ESP_OK;
}

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

}