#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
// #include "driver/gpio.h"
#include "driver/spi_master.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_dev.h"
#include "esp_err.h"      
#include "esp_lcd_panel_ops.h"
// My libraries
#include "globals.h"
#include "functions.h"



void app_main(void) {

    // First we need to initialize SPI bus
    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO));

    // Attach the LCD to the SPI bus
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)SPI3_HOST, &io_config, &io_handle));

    // Create LCD panel handle for ST7789, with the SPI IO device handle
    ESP_ERROR_CHECK(esp_lcd_new_panel_st7735(io_handle, &panel_config, &panel_handle));

    xTaskCreate(LCD_Display_Task, "Display_LCD", 4096, NULL, 5, NULL);
}