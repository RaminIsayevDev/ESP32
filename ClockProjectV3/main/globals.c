#include "globals.h"
#include <stdint.h>  
#include <stdbool.h>
#include <time.h>

#include "lvgl.h"
#include "../lvgl-9.3.0/src/drivers/display/st7735/lv_st7735.h"
#include "driver/gpio.h"

SemaphoreHandle_t lvgl_mutex;

spi_device_handle_t display_spi_handle;

lv_obj_t *main_screen = NULL;
lv_obj_t *clock_label = NULL;
lv_obj_t *temp_label = NULL;
lv_style_t clock_label_style;
lv_style_t temp_label_style;

QueueHandle_t toDisplay_Queue;
QueueHandle_t SNTP_to_RTC_Queue;

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

struct tm RTC_timeinfo = {0};

const char *TAG = "wifi_task";
const char *TAG_IP = "ip_event";
const char *TAG_SPLASH = "Splash_Screen";

bool sntp_started = false;

float temp = 0;

const uint8_t GAMMA_P_ARRAY_LEN  = sizeof(GAMMA_P_ARRAY);
const uint8_t GAMMA_N_ARRAY_LEN  = sizeof(GAMMA_N_ARRAY);