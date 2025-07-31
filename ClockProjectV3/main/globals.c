#include "globals.h"
#include <stdint.h>  
#include <stdbool.h>
#include <time.h>

#include "lvgl.h"
#include "st7735s.h"
#include "drivers/gpio.h"

QueueHandle_t toDisplay_Queue;
QueueHandle_t SNTP_to_RTC_Queue;



i2c_master_bus_handle_t i2c_bus = NULL;

// Инициализация D/C пина
gpio_config_t io_conf = {};
io_conf.pin_bit_mask = (1ULL << PIN_NUM_DC) | (1ULL << PIN_NUM_RST) | (1ULL << PIN_NUM_BCKL);
io_conf.mode = GPIO_MODE_OUTPUT;
io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
io_conf.intr_type = GPIO_INTR_DISABLE;
gpio_config(&io_conf);

// Управление пином RST (выполняется один раз при инициализации дисплея)
gpio_set_level(PIN_NUM_RST, 0); // Active low reset
vTaskDelay(pdMS_TO_TICKS(100)); // Hold low for a bit
gpio_set_level(PIN_NUM_RST, 1); // Release reset
vTaskDelay(pdMS_TO_TICKS(100)); // Wait for display to come up

// Управление подсветкой (если есть)
gpio_set_level(PIN_NUM_BCKL, 1); // Включить подсветку (если активный высокий)

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