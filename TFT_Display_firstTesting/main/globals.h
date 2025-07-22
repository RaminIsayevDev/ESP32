#ifndef UTILS_H
#define UTILS_H

#include "driver/gpio.h"
#include "esp_lcd_panel_dev.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"

// Define Pins

extern const gpio_num_t SCLK_PIN;
extern const gpio_num_t MISO_PIN;
extern const gpio_num_t MOSI_PIN;
extern const gpio_num_t CS_PIN;
extern const gpio_num_t RST_PIN;
extern const gpio_num_t DC_PIN;
extern const gpio_num_t BKL_PIN;

// Define Variables

extern const uint8_t ST7735S_GMCTRP1_CMD;
extern const uint8_t ST7735S_GMCTRN1_CMD;

extern const int LCD_H_RES;
extern const int LCD_V_RES;

extern const uint8_t GAMMA_P_ARRAY_LEN;
extern const uint8_t GAMMA_N_ARRAY_LEN;

extern esp_lcd_panel_io_handle_t io_handle;
extern esp_lcd_panel_handle_t panel_handle;

extern const int LINE_BUFFER_HEIGHT;

// Define structures and arrays

extern const uint8_t GAMMA_P_ARRAY[];
extern const uint8_t GAMMA_N_ARRAY[];

extern const spi_bus_config_t bus_config;
extern const esp_lcd_panel_io_spi_config_t io_config;
extern const esp_lcd_panel_dev_config_t panel_config;

#endif