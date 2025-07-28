#include "globals.h"
#include <stdint.h>  
#include <stdbool.h>
#include <time.h>

#include "lvgl.h"
#include "st7735s.h"

QueueHandle_t toDisplay_Queue;
QueueHandle_t SNTP_to_RTC_Queue;

lv_disp_drv_t disp_drv;

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

bool sntp_started = false;

float temp = 0;

const uint8_t GAMMA_P_ARRAY_LEN  = sizeof(GAMMA_P_ARRAY);
const uint8_t GAMMA_N_ARRAY_LEN  = sizeof(GAMMA_N_ARRAY);