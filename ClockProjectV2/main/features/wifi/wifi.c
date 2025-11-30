#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "driver/gpio.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_log.h"

#include "wifi.h"
#include "features/sntp/sntp.h"

const char *TAG_IP = "ip_event";
const char *TAG = "wifi_task";
bool sntp_started = false;

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

void my_wifi_disconnected_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    wifi_event_sta_disconnected_t* disconn = (wifi_event_sta_disconnected_t*) event_data;
    ESP_LOGW(TAG, "Disconnected from Wi-Fi. Reason: %d", disconn->reason);
}

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

    // Настройка параметров подключения
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "ALHN-3405",
            .password = "0554540509"
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));

    // Запуск Wi-Fi
    ESP_ERROR_CHECK(esp_wifi_start());

    // Задача завершена, так как все события будут обрабатываться через event-хендлеры
    vTaskDelete(NULL);
}