#ifndef GLOBALS_H
#define GLOBALS_H

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include "lvgl.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"
#include "driver/i2c_master.h"
#include "driver/spi_master.h"

#include "stdbool.h"

extern SemaphoreHandle_t lvgl_mutex;

#define lvgl_safe_call(code_block)                  \
    do {                                            \
        if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY)) { \
            code_block;                             \
            xSemaphoreGive(lvgl_mutex);    \
        }                                           \
    } while (0)

extern spi_device_handle_t display_spi_handle;

extern i2c_master_bus_handle_t* i2c_bus;
// Queue

extern QueueHandle_t toDisplay_Queue;
extern QueueHandle_t SNTP_to_RTC_Queue;

// define pins for SPI

#define SCLK_PIN 18
#define MISO_PIN 19
#define MOSI_PIN 23
#define CS_PIN 5
#define RST_PIN 16
#define DC_PIN 17
#define BKL_PIN 4

// define pins for I2C

#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22

// Define structures and arrays

extern const uint8_t GAMMA_P_ARRAY[];
extern const uint8_t GAMMA_N_ARRAY[];

// Data for the toDisplay_Queue

struct toDisplay_data {
    int hour;
    int min;
    float temp;
};

// Tags

extern const char *TAG;
extern const char *TAG_IP;
extern const char *TAG_SPLASH;

// For main_screen

extern lv_obj_t *main_screen;
extern lv_obj_t *clock_label;
extern lv_obj_t *temp_label;
extern lv_style_t clock_label_style;
extern lv_style_t temp_label_style;

// Flag for control SNTP_start

extern bool sntp_started;

// Length of GAMMMA arrays

extern const uint8_t GAMMA_P_ARRAY_LEN;
extern const uint8_t GAMMA_N_ARRAY_LEN;

//extern struct tm RTC_timeinfo;

#endif