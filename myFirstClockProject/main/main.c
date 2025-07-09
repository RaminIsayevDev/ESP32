#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_sntp.h"
#include <time.h>

static const char *TAG = "wifi_task";
static const char *TAG_IP = "ip_event";

void my_wifi_connected_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_id == WIFI_EVENT_STA_CONNECTED) {
    wifi_event_sta_connected_t* connected_info = (wifi_event_sta_connected_t*) event_data;
    printf("Connected to: %.*s, channel: %d\n", connected_info->ssid_len, connected_info->ssid, connected_info->channel);
  } else {
    printf("Can't connect to Wi-Fi...\n");  
  }
}

void my_wifi_started_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
  if (event_id == WIFI_EVENT_STA_START) {
    printf("Wi-Fi driver succesfully started. Trying to connect...\n");
    ESP_ERROR_CHECK(esp_wifi_connect());
  } else {
    printf("Error while trying to start Wi-Fi driver.\n");
  }
}

static void my_wifi_disconnected_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    wifi_event_sta_disconnected_t* disconn = (wifi_event_sta_disconnected_t*) event_data;
    ESP_LOGW(TAG, "Disconnected from Wi-Fi. Reason: %d", disconn->reason);
}

// Добавьте глобальную переменную для контроля запуска SNTP
static bool sntp_started = false;
void sntp_task(void *pvParameters);

void my_wifi_ip_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_id == IP_EVENT_STA_GOT_IP && event_base == IP_EVENT) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;

        ESP_LOGI(TAG_IP, "Got IP address!");
        ESP_LOGI(TAG_IP, "--------------------------------------------------");
        ESP_LOGI(TAG_IP, "IP address:   " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG_IP, "Netmask:      " IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGI(TAG_IP, "Gateway:      " IPSTR, IP2STR(&event->ip_info.gw));
        ESP_LOGI(TAG_IP, "--------------------------------------------------");

        // Запускаем SNTP только один раз после получения IP
        if (!sntp_started) {
            sntp_started = true;
            xTaskCreate(sntp_task, "sntp_task", 4096, NULL, 5, NULL);
        }
    } else {
        printf("Can't receive the IP...");
    }
}

void wifi_task(void *pvParameters) {
  // Инициализация NVS
  esp_err_t ret = nvs_flash_init();
  if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ESP_ERROR_CHECK(nvs_flash_init());
  }

  // Инициализация Wi-Fi
  ESP_ERROR_CHECK(esp_netif_init());
  esp_netif_create_default_wifi_sta();
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
    .cc = "US",
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
      ESP_LOGI(TAG, "Connected to Wi-Fi\n");
      break;
    }
    vTaskDelay(1000 / portTICK_PERIOD_MS);
  }

  // Поддержание соединения
  while (1) {
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) != ESP_OK) {
      ESP_LOGI(TAG, "Disconnected from Wi-Fi, reconnecting...\n");
      ESP_ERROR_CHECK(esp_wifi_connect());
    }
    vTaskDelay(5000 / portTICK_PERIOD_MS);
  }

}

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
    } else {
        ESP_LOGW("sntp_task", "Failed to synchronize time");
    }

  // Периодически выводим текущее время
    while (1) {
        time(&now);
        localtime_r(&now, &timeinfo);
        ESP_LOGI("sntp_task", "Current time: %s", asctime(&timeinfo));
        vTaskDelay(60000 / portTICK_PERIOD_MS); // 1 минута
    }
}

void app_main(void) {
// Слушатель всех event-ов
  ESP_ERROR_CHECK(esp_event_loop_create_default());
  
// Register event handler after event loop is created
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &my_wifi_connected_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &my_wifi_started_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &my_wifi_disconnected_handler, NULL));
  ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &my_wifi_ip_handler, NULL));

// Создание задачи Wi-Fi
  xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 10, NULL);

}