#ifndef FUNCTIONS_H
#define FUNCTIONS_H

#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_dev.h"

void send_gamma_config(esp_lcd_panel_io_handle_t io_handle);

void LCD_Display_Task(void* pvParameters);

void fill_screen_blue_by_strips(esp_lcd_panel_handle_t panel_handle);

#endif