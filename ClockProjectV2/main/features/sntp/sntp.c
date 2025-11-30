#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "driver/gpio.h"
#include "lwip/apps/sntp.h"
#include "esp_log.h"
#include <time.h>

#include "core/queue/queue.h"
#include "sntp.h"

void sntp_task(void *pvParameters) {
  // Настройка SNTP
    ESP_LOGI("sntp_task", "Initializing SNTP");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);
    sntp_setservername(0, "time.google.com");
    sntp_init();

    setenv("TZ", "GMT-4", 1);
    tzset();

  // Ожидание синхронизации времени
    time_t now = 0;
    struct tm timeinfo = { 0 };
    int retry = 0;
    const int retry_count = 10;
    while (timeinfo.tm_year < (2016 - 1900) && ++retry < retry_count) {
        ESP_LOGI("sntp_task", "Waiting for system time to be set... (%d/%d)", retry, retry_count);
        vTaskDelay(2000 / portTICK_PERIOD_MS);
        time(&now);
        localtime_r(&now, &timeinfo);
    }

    if (timeinfo.tm_year >= (2016 - 1900)) {
        ESP_LOGI("sntp_task", "Time synchronized: %s", asctime(&timeinfo));
        xQueueSend(queue_SNTP_to_RTC_data, &timeinfo, portMAX_DELAY);
        xTaskCreate(RTC_task, "RTC task", 2048, NULL, 10, NULL);
    } else {
        ESP_LOGW("sntp_task", "Failed to synchronize time");
    }

    while(1) {
      xQueueSend(queue_SNTP_to_RTC_data, &timeinfo, portMAX_DELAY); 
      vTaskDelay(pdMS_TO_TICKS(3600000));  // 3600000 миллисекунд = 1 час
    }
}