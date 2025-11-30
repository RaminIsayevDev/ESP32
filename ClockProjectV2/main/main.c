#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp-idf-ds3231.h"
#include "driver/i2c_master.h"
#include <time.h>
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "tm1637.h"

#include "features/wifi/wifi.h"
#include "features/rtc/rtc.h"
#include "core/queue/queue.h"
#include "features/display/display.h"

void display_task(void *pvParameters);

QueueHandle_t queue_SNTP_to_RTC_data;

tm1637_led_t *led = NULL;  // Глобальная переменная

void app_main(void) {
// Слушатель всех event-ов
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  
// Register event handler after event loop is created
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &my_wifi_connected_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &my_wifi_started_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &my_wifi_disconnected_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &my_wifi_ip_handler, NULL));

// Создание очередей
  queue_SNTP_to_RTC_data = xQueueCreate(5, sizeof(struct tm));

// Инициализация TM1637
  led = tm1637_init(GPIO_NUM_18, GPIO_NUM_19);
  tm1637_set_brightness(led, 1);

// Создание задачи Wi-Fi
  xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 10, NULL);
}