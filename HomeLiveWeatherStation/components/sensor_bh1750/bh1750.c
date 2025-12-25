#include "bh1750.h"
#include "esp_log.h"
#include "driver/i2c.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "BH1750";

/**
 * @brief Initialize BH1750 sensor
 */
esp_err_t bh1750_init(bh1750_t *dev, uint8_t i2c_num) {
    if (dev == NULL) {
        ESP_LOGE(TAG, "Device structure is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    dev->i2c_addr = BH1750_I2C_ADDR;
    dev->mode = BH1750_CONTINUOUS_HIGH_RES;
    dev->light_level = 0;

    // --- 1. Power on the sensor ---
    uint8_t power_on_cmd = 0x01;
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (dev->i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd_handle, power_on_cmd, true);
    i2c_master_stop(cmd_handle);
    
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd_handle, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd_handle);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to power on BH1750: %s", esp_err_to_name(ret));
        return ret;
    }
    vTaskDelay(pdMS_TO_TICKS(10)); // Wait a bit after power on

    // --- 2. Configure sensor for continuous high resolution mode ---
    uint8_t mode_cmd = dev->mode;
    cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (dev->i2c_addr << 1) | I2C_MASTER_WRITE, true);
    i2c_master_write_byte(cmd_handle, mode_cmd, true);
    i2c_master_stop(cmd_handle);

    ret = i2c_master_cmd_begin(i2c_num, cmd_handle, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd_handle);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set mode for BH1750: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Wait for sensor to complete first measurement
    vTaskDelay(pdMS_TO_TICKS(200));
    ESP_LOGI(TAG, "BH1750 initialized successfully");
    return ESP_OK;
}

/**
 * @brief Read light level from BH1750 sensor
 */
uint16_t bh1750_read_light_level(bh1750_t *dev, uint8_t i2c_num) {
    if (dev == NULL) {
        ESP_LOGE(TAG, "Device structure is NULL");
        return ESP_ERR_INVALID_ARG;
    }

    uint8_t data[2] = {0};
    
    // Read 2 bytes from sensor
    i2c_cmd_handle_t cmd_handle = i2c_cmd_link_create();
    i2c_master_start(cmd_handle);
    i2c_master_write_byte(cmd_handle, (dev->i2c_addr << 1) | I2C_MASTER_READ, true);
    i2c_master_read_byte(cmd_handle, &data[0], I2C_MASTER_ACK);
    i2c_master_read_byte(cmd_handle, &data[1], I2C_MASTER_NACK);
    i2c_master_stop(cmd_handle);
    
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd_handle, pdMS_TO_TICKS(1000));
    i2c_cmd_link_delete(cmd_handle);
    
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to read BH1750: %s", esp_err_to_name(ret));
        return ret;
    }
    
    // Convert raw data to lux value
    // Formula: lux = (MSB << 8 | LSB) / 1.2
    dev->light_level = ((data[0] << 8) | data[1]) / 1.2;
    
    return dev->light_level;
}

/**
 * @brief Get last read light level
 */
uint16_t bh1750_get_light_level(bh1750_t *dev) {
    if (dev == NULL) {
        return 0;
    }
    return dev->light_level;
}