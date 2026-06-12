#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include "spi/include/spi_init.h"
#include "spi/include/spi_pins.h"

esp_err_t init_spi_bus(void) {
    spi_bus_config_t buscfg = {
        .sclk_io_num = PIN_NUM_CLK,
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = -1, // Для ST7735S чтение обычно не нужно
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        // max_transfer_sz = full screen buffer for DMA
        .max_transfer_sz = LCD_H_RES * LCD_V_RES * sizeof(uint16_t),
    };

    // Инициализируем только шину! Автоматический выбор канала DMA
    return spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
}
