#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>
#include "esp_event.h"
#include "driver/spi_master.h"

void RTC_Task(void* pvParameters);
void sntp_task(void *pvParameters);
void wifi_task(void *pvParameters);
void lvgl_main_task(void *pvParameters);
void display_task(void* pvParameters);

void st7735s_spi_pre_transfer_callback(spi_transaction_t *t);
void init_display_pins(void);
void my_wifi_connected_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void my_wifi_started_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void my_wifi_disconnected_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void my_wifi_ip_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

#endif