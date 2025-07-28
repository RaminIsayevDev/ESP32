#ifndef TASKS_H
#define TASKS_H

#include <stdint.h>

#include "esp_event.h"

void RTC_Task(void* pvParameters);
void sntp_task(void *pvParameters);
void wifi_task(void *pvParameters);
void display_task(void* pvParameters);

void my_wifi_connected_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void my_wifi_started_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void my_wifi_disconnected_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void my_wifi_ip_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data);

#endif