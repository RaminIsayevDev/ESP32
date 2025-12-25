#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"
#include "i2c_bus.h"
#include "driver/i2c.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_err.h"
#include "task_sensors.h"
#include "wifi_manager.h"

#define I2C_MASTER_NUM 0  // Определение I2C_MASTER_NUM

static const char *TAG = "MAIN";

void app_main(void) {
    wifi_manager_init();
    // Инициализируем шину I2C
    if (i2c_master_init() == ESP_OK) {
        ESP_LOGI(TAG, "I2C bus initialized successfully");
    } else {
        ESP_LOGE(TAG, "Failed to initialize I2C bus");
        return;
    }

    sensors_init();
    
}