#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"

#include "i2c_bus.h"

#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM 0
#define I2C_MASTER_FREQ_HZ 100000

static const char *TAG = "I2C-SCANNER";

esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;

    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE, // Включаем внутренние подтяжки
        .scl_pullup_en = GPIO_PULLUP_ENABLE, // Включаем внутренние подтяжки
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
        // .clk_flags = 0, // Опционально, можно оставить по умолчанию
    };

    // 1. Применяем конфигурацию
    esp_err_t err = i2c_param_config(i2c_master_port, &conf);
    if (err != ESP_OK) {
        return err;
    }

    // 2. Устанавливаем драйвер
    // Аргументы: порт, режим, размер буфера RX, размер буфера TX, флаги прерываний
    return i2c_driver_install(i2c_master_port, conf.mode,
                              0,  // RX буфер (0 для Master)
                              0,  // TX буфер (0 для Master)
                              0); // Флаги (0 = по умолчанию)
}