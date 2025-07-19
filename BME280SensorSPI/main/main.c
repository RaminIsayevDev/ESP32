#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c.h"
#include "esp_log.h"

#include "bme280.h"  // Официальный SDK Bosch
#include "rom/ets_sys.h"

#define I2C_MASTER_NUM        I2C_NUM_0
#define I2C_MASTER_SDA_IO     19
#define I2C_MASTER_SCL_IO     18
#define I2C_MASTER_FREQ_HZ    100000
#define BMP280_I2C_ADDR       0x76 // Или 0x77 в зависимости от CSB

static const char *TAG = "BMP280_APP";

void delay_us(uint32_t period_us, void *intf_ptr) {
    ets_delay_us(period_us);
}

int8_t i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    uint8_t dev_addr = *(uint8_t *)intf_ptr;
    esp_err_t ret = i2c_master_write_read_device(I2C_MASTER_NUM, dev_addr, &reg_addr, 1, reg_data, len, pdMS_TO_TICKS(1000));
    return (ret == ESP_OK) ? BME280_OK : BME280_E_COMM_FAIL;
}

int8_t i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr) {
    uint8_t dev_addr = *(uint8_t *)intf_ptr;
    uint8_t buffer[1 + len];
    buffer[0] = reg_addr;
    memcpy(&buffer[1], reg_data, len);
    esp_err_t ret = i2c_master_write_to_device(I2C_MASTER_NUM, dev_addr, buffer, sizeof(buffer), pdMS_TO_TICKS(1000));
    return (ret == ESP_OK) ? BME280_OK : BME280_E_COMM_FAIL;
}

void sensor_task(void *pvParameter) {
    struct bme280_dev *dev = (struct bme280_dev *)pvParameter;
    struct bme280_settings settings;
    struct bme280_data comp_data;

    if (bme280_get_sensor_settings(&settings, dev) != BME280_OK) {
        ESP_LOGE(TAG, "Failed to get sensor settings");
        vTaskDelete(NULL);
        return;
    }

    settings.osr_t = BME280_OVERSAMPLING_2X;
    settings.osr_p = BME280_OVERSAMPLING_16X;
    settings.osr_h = BME280_NO_OVERSAMPLING; // Для BMP280 это не используется
    settings.filter = BME280_FILTER_COEFF_16;

    uint8_t settings_sel = BME280_SEL_OSR_TEMP | BME280_SEL_OSR_PRESS | BME280_SEL_FILTER;
    if (bme280_set_sensor_settings(settings_sel, &settings, dev) != BME280_OK) {
        ESP_LOGE(TAG, "Failed to set sensor settings");
        vTaskDelete(NULL);
        return;
    }

    ESP_LOGI(TAG, "Sensor configured successfully");

    while (1) {
        if (bme280_set_sensor_mode(BME280_POWERMODE_FORCED, dev) != BME280_OK) {
            ESP_LOGE(TAG, "Failed to set sensor mode");
        } else {
            uint32_t delay_us_val;
            bme280_cal_meas_delay(&delay_us_val, &settings);
            dev->delay_us(delay_us_val, dev->intf_ptr);

            if (bme280_get_sensor_data(BME280_PRESS | BME280_TEMP, &comp_data, dev) == BME280_OK) {
                ESP_LOGI(TAG, "Temp: %.2f C, Pressure: %.2f hPa",
                         comp_data.temperature, comp_data.pressure / 100.0);
            } else {
                ESP_LOGE(TAG, "Failed to read sensor data");
            }
        }
        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main() {
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_DISABLE,
        .scl_pullup_en = GPIO_PULLUP_DISABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ
    };
    i2c_param_config(I2C_MASTER_NUM, &conf);
    i2c_driver_install(I2C_MASTER_NUM, I2C_MODE_MASTER, 0, 0, 0);

    static struct bme280_dev dev;
    static uint8_t addr = BMP280_I2C_ADDR;

    dev.intf = BME280_I2C_INTF;
    dev.read = i2c_read;
    dev.write = i2c_write;
    dev.delay_us = delay_us;
    dev.intf_ptr = &addr;

    if (bme280_init(&dev) != BME280_OK) {
        ESP_LOGE(TAG, "Failed to initialize sensor");
        return;
    }

    xTaskCreate(sensor_task, "sensor_task", 4096, &dev, 5, NULL);
}
