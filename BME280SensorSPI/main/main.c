#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h" // For ESP-IDF 5.x I2C master driver
#include "esp_log.h"
#include "esp_err.h"

// Include the specific header for the k0i05/esp_bmp280 component
#include "bmp280.h" // This is based on the source file you provided

#define I2C_MASTER_NUM        I2C_NUM_0
#define I2C_MASTER_SDA_IO     19
#define I2C_MASTER_SCL_IO     18
#define I2C_MASTER_FREQ_HZ    100000
#define BMP280_I2C_ADDR       0x76 // Or 0x77 depending on CSB

static const char *TAG = "BMP280_APP";

// The k0i05/esp_bmp280 library handles I2C communication and delays internally.
// You no longer need these custom functions:
// void delay_us(uint32_t period_us, void *intf_ptr) { ... }
// int8_t i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr) { ... }
// int8_t i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr) { ... }


void sensor_task(void *pvParameter) {
    // Cast the parameter to the correct handle type for this driver
    bmp280_handle_t bmp280_dev_handle = (bmp280_handle_t)pvParameter;
    float temperature = 0.0f;
    float pressure = 0.0f;
    esp_err_t ret;

    ESP_LOGI(TAG, "Sensor task started. Reading data...");

    while (1) {
        // Use the bmp280_get_measurements function from the component
        ret = bmp280_get_measurements(bmp280_dev_handle, &temperature, &pressure);

        if (ret == ESP_OK) {
            ESP_LOGI(TAG, "Temp: %.2f C, Pressure: %.2f hPa",
                     temperature, pressure / 100.0f); // Pressure is in Pa, convert to hPa
        } else {
            ESP_LOGE(TAG, "Failed to read sensor data (Error: %s)", esp_err_to_name(ret));
            // Depending on the error, you might want to re-initialize or reset the sensor.
            // For now, we'll just log and continue.
        }

        vTaskDelay(pdMS_TO_TICKS(5000)); // Read every 5 seconds
    }
}

void app_main() {
    esp_err_t ret;

    // --- 1. Initialize I2C Master Bus ---
    // This is crucial for ESP-IDF 5.x and how k0i05/esp_bmp280 expects the I2C handle.
    i2c_master_bus_config_t i2c_bus_config = {
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7, // Default is usually fine
        .flags.enable_internal_pullup = true, // Set to true if you don't have external pull-ups
    };
    i2c_master_bus_handle_t master_i2c_bus_handle;

    ret = i2c_new_master_bus(&i2c_bus_config, &master_i2c_bus_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create I2C master bus: %s", esp_err_to_name(ret));
        return;
    }
    ESP_LOGI(TAG, "I2C master bus initialized successfully.");

    // --- 2. Configure BMP280 Sensor Parameters ---
    bmp280_config_t bmp280_cfg = {
        .i2c_address              = BMP280_I2C_ADDR,
        .i2c_clock_speed          = I2C_MASTER_FREQ_HZ,
        .power_mode               = BMP280_POWER_MODE_FORCED, // Use forced mode for single measurements
        .temperature_oversampling = BMP280_TEMPERATURE_OVERSAMPLING_2X,
        .pressure_oversampling    = BMP280_PRESSURE_OVERSAMPLING_16X,
        .standby_time             = BMP280_STANDBY_TIME_500MS, // This will be used if in Normal mode, but good to set
        .iir_filter               = BMP280_IIR_FILTER_16,
    };

    // --- 3. Initialize BMP280 Device Handle ---
    bmp280_handle_t bmp280_dev_handle = NULL; // Initialize to NULL
    ret = bmp280_init(master_i2c_bus_handle, &bmp280_cfg, &bmp280_dev_handle);

    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize BMP280 sensor: %s", esp_err_to_name(ret));
        // Clean up I2C bus if sensor init fails
        i2c_del_master_bus(master_i2c_bus_handle);
        return;
    }
    ESP_LOGI(TAG, "BMP280 sensor initialized successfully.");

    // --- 4. Create and Start the Sensor Task ---
    // Pass the bmp280_handle_t to the task
    if (xTaskCreate(sensor_task, "sensor_task", 4096, bmp280_dev_handle, 5, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Failed to create sensor task.");
        // Clean up BMP280 and I2C if task creation fails
        bmp280_delete(bmp280_dev_handle); // This also removes it from the bus
        i2c_del_master_bus(master_i2c_bus_handle);
        return;
    }
}