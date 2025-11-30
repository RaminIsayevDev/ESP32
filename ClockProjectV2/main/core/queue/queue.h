#ifndef QUEUE.H
#define QUEUE.H

#include <freertos/FreeRTOS.h>
#include "freertos/task.h"

extern QueueHandle_t queue_SNTP_to_RTC_data;
void RTC_task(void *pvParameters);

#endif