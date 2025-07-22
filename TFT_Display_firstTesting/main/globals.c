#include "functions.h"
#include "globals.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_dev.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "driver/spi_master.h"

// Pins

const gpio_num_t SCLK_PIN = GPIO_NUM_18;
const gpio_num_t MISO_PIN = GPIO_NUM_19;
const gpio_num_t MOSI_PIN = GPIO_NUM_23;
const gpio_num_t CS_PIN = GPIO_NUM_5;
const gpio_num_t RST_PIN = GPIO_NUM_16;
const gpio_num_t DC_PIN = GPIO_NUM_17;
const gpio_num_t BKL_PIN = GPIO_NUM_22;

// Variables

ST7735S_GMCTRN1_CMD = 0xE1;
ST7735S_GMCTRP1_CMD = 0xE0;

LCD_H_RES = 128;
LCD_V_RES = 160;

GAMMA_P_ARRAY_LEN  = sizeof(GAMMA_P_ARRAY);
GAMMA_N_ARRAY_LEN  = sizeof(GAMMA_N_ARRAY);

esp_lcd_panel_io_handle_t io_handle = NULL;
esp_lcd_panel_handle_t panel_handle = NULL;

LINE_BUFFER_HEIGHT = 10;

// Structures, arrays

const uint8_t GAMMA_P_ARRAY[] = {
    0x02, 0x1c, 0x07, 0x12,
    0x37, 0x32, 0x29, 0x2d,
    0x25, 0x2B, 0x39, 0x00,
    0x01, 0x03, 0x10, 0x10 
};

const uint8_t GAMMA_N_ARRAY[] = {
    0x03, 0x1d, 0x07, 0x06,
    0x2E, 0x2C, 0x29, 0x2D,
    0x25, 0x2D, 0x3B, 0x00,
    0x00, 0x01, 0x10, 0x10 
};

spi_bus_config_t bus_config = {
        .sclk_io_num = SCLK_PIN,
        .miso_io_num = MISO_PIN,
        .mosi_io_num = MOSI_PIN,
        .quadhd_io_num = -1,
        .quadwp_io_num = -1,
        .max_transfer_sz = 160 * 80 * sizeof(uint16_t),
};

esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = DC_PIN,
        .cs_gpio_num = CS_PIN,
        .pclk_hz = 20 * 1000 * 1000, // 20 MHz
        .lcd_cmd_bits = 8,
        .lcd_param_bits = 8,
        .spi_mode = 0,
        .trans_queue_depth = 10,
};

esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = RST_PIN,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
};