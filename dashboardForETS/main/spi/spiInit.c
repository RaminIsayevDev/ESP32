#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include "include/spi_pins.h"
#include "st7735s.h"
#include "gfx.h"
#include "fonts.h"
#include "include/spi_init.h"

esp_err_t spi_init() {
    esp_err_t ret;

    // Конфигурируем GPIO для DC и RST
    gpio_reset_pin(PIN_NUM_DC);
    gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);
    gpio_reset_pin(PIN_NUM_RST);
    gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);

    spi_bus_config_t buscfg = {
        .miso_io_num = PIN_NUM_MISO,
        .mosi_io_num = PIN_NUM_MOSI,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 0
    };

    ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
    if (ret != ESP_OK) {
        printf("Failed to initialize SPI bus: %s\n", esp_err_to_name(ret));
        return ret;
    }

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 10 * 1000 * 1000, // 10 MHz
        .mode = 0, // SPI mode 0
        .queue_size = 7,
        .spics_io_num = PIN_NUM_CS,
    };
    
    ret = spi_bus_add_device(LCD_HOST, &devcfg, &spi_handle);
    if (ret != ESP_OK) {
        printf("Failed to add SPI device: %s\n", esp_err_to_name(ret));
        return ret;
    }
    return ESP_OK;
}

// 1. Реализуем функцию отправки команды через SPI драйвер ESP-IDF
void my_spi_write_cmd(uint8_t cmd) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &cmd;
    gpio_set_level(PIN_NUM_DC, 0); // DC = 0 (Команда)
    spi_device_polling_transmit(spi_handle, &t);
}

// 2. Реализуем функцию отправки данных через SPI драйвер ESP-IDF
void my_spi_write_data(uint8_t data) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));
    t.length = 8;
    t.tx_buffer = &data;
    gpio_set_level(PIN_NUM_DC, 1); // DC = 1 (Данные)
    spi_device_polling_transmit(spi_handle, &t);
}