#ifndef GLOBALS_H
#define GLOBALS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "lvgl.h"
#include "st7735s.h"

#include "stdbool.h"

// Queue

extern QueueHandle_t toDisplay_Queue;
extern QueueHandle_t SNTP_to_RTC_Queue;

// LVGL Labels

extern lv_obj_t* wifi_label;
extern lv_obj_t* clock_label;
extern lv_obj_t* temp_label;
extern lv_obj_t* ssid_label;

// Buffers

#define DISP_BUF_LINES 40
#define LVGL_BUF_SIZE (LV_HOR_RES_MAX * DISP_BUF_LINES)

// define pins for SPI

#define SCLK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 23
#define CS_PIN 5
#define RST_PIN 16
#define DC_PIN 17
#define BKL_PIN 24

// Displey driver

extern lv_disp_drv_t disp_drv;

// Define structures and arrays

extern const uint8_t GAMMA_P_ARRAY[];
extern const uint8_t GAMMA_N_ARRAY[];

// Data for the toDisplay_Queue

typedef enum {
    DISPLAY_WIFI_STATUS,
    DISPLAY_CLOCK_TEMP
} DisplayMessageType;

typedef struct {
    DisplayMessageType type;
    union {
        struct {
            bool connected;
            char ssid[32];
        } wifi_status;

        struct {
            char time[16];  // "12:34:56"
            char temp[16];  // "23.5 C"
        } clock_temp;
    } data;
} DisplayMessage;

// Tags

extern const char *TAG;
extern const char *TAG_IP;

// Flag for control SNTP_start

extern bool sntp_started;

// Length of GAMMMA arrays

extern const uint8_t GAMMA_P_ARRAY_LEN;
extern const uint8_t GAMMA_N_ARRAY_LEN;

//extern struct tm RTC_timeinfo;

#endif