#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"

static const char *TAG = "wifi_task";

void my_wifi_connected_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_id == WIFI_EVENT_STA_CONNECTED) {
    wifi_event_sta_connected_t* connected_info = (wifi_event_sta_connected_t*) event_data;
    printf("Connected to: %.*s, channel: %d\n", connected_info->ssid_len, connected_info->ssid, connected_info->channel);
  } else {
    printf("Can't connect to Wi-Fi...");  
  }
}

void my_wifi_started_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_id == WIFI_EVENT_STA_START) {
    printf("Wi-Fi driver succesfully started. Trying to connect...");
    ESP_ERROR_CHECK(esp_wifi_connect());
  } else {
    printf("Error while trying to start Wi-Fi driver.");
  }
}

void wifi_task(void *pvParameters) {
  // Инициализация Wi-Fi
  ESP_ERROR_CHECK(esp_netif_init());
  ESP_ERROR_CHECK(esp_netif_create_default_wifi_sta());
  wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();

  ESP_ERROR_CHECK(esp_wifi_init(&cfg));
  ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

  // Настройка параметров подключения
  wifi_config_t wifi_config = {
    .sta = {
      .ssid = "ALHN-3405",
      .password = "0554540509"
    },
  };

  wifi_country_t country = {
    .cc = "AZ",
    .schan = 1,
    .nchan = 13,
    .policy = WIFI_COUNTRY_POLICY_AUTO
  };

  ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

  ESP_ERROR_CHECK(esp_wifi_set_country(&country));

  // Запуск Wi-Fi
  ESP_ERROR_CHECK(esp_wifi_start());

  // Ожидание подключения
  while (1) {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
      ESP_LOGI(TAG, "Connected to Wi-Fi");
      break;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  // Поддержание соединения
  while (1) {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
      ESP_LOGI(TAG, "Disconnected from Wi-Fi, reconnecting...");
      ESP_ERROR_CHECK(esp_wifi_connect());
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }

}

void app_main(void) {
// Слушатель всех event-ов
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  
// Register event handler after event loop is created
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &my_wifi_connected_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &my_wifi_started_handler, NULL));

// Создание задачи Wi-Fi
  xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 5, NULL);
}