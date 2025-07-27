#ifndef UTILS_H
#define UTILS_H

#include "driver/gpio.h"
#include "esp_lcd_panel_dev.h"
#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "lvgl.h"
#include "lvgl_helpers.h"

// Define Pins

#define SCLK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 23
#define CS_PIN 5
#define RST_PIN 16
#define DC_PIN 17
#define BKL_PIN 22

// Define Variables

extern const int BUFFER_SIZE;

extern lv_disp_drv_t disp_drv;

extern const uint8_t GAMMA_P_ARRAY_LEN;
extern const uint8_t GAMMA_N_ARRAY_LEN;

extern const int DISP_BUF_LINES;

// Define structures and arrays

extern const uint8_t GAMMA_P_ARRAY[];
extern const uint8_t GAMMA_N_ARRAY[];

extern const spi_bus_config_t bus_config;
extern const esp_lcd_panel_io_spi_config_t io_config;
extern const esp_lcd_panel_dev_config_t panel_config;

#endif