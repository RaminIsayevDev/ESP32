#include "i2c_bus.h" // Подключаем наш новый заголовок
#include "esp_log.h"

static const char *TAG = "MAIN";

void app_main(void)
{
    // 1. Инициализируем шину
    if (i2c_master_init() == ESP_OK) {
        ESP_LOGI(TAG, "I2C bus initialized successfully");
    } else {
        ESP_LOGE(TAG, "Failed to initialize I2C bus");
        return; // Дальше нет смысла идти, если шина не работает
    }

    // 2. Тут потом будет инициализация сенсоров
    // bme280_init();
}