#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp-idf-ds3231.h"
#include "driver/i2c_master.h"
#include <time.h>
#include "nvs_flash.h"
#include "lwip/apps/sntp.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "tm1637.h"

#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22

static const char *TAG = "wifi_task";
static const char *TAG_IP = "ip_event";
static bool sntp_started = false;
i2c_master_bus_handle_t i2c_bus = NULL;
void sntp_task(void *pvParameters);
void RTC_task(void *pvParameters);
void display_task(void *pvParameters);
QueueHandle_t queue_SNTP_to_RTC_data;
tm1637_led_t *led = NULL;  // Глобальная переменная
struct tm RTC_timeinfo = {0};

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

void RTC_task(void *pvParameters) { // Task for set, save and output time when needed

  i2c_master_bus_config_t i2c_bus_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_NUM_0,
    .scl_io_num = SCL_PIN,
    .sda_io_num = SDA_PIN,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
  };

  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus));

  rtc_handle_t *rtc = ds3231_init(&i2c_bus);

  
  float temp = 0;

  if (xQueueReceive(queue_SNTP_to_RTC_data, &RTC_timeinfo, portMAX_DELAY) == pdTRUE) {
    //printf("Received: %d\n", received_value);
    ESP_ERROR_CHECK(ds3231_time_tm_set(rtc, RTC_timeinfo));
    xTaskCreate(display_task, "display_time", 1024, NULL, 10, NULL);
  }

  while(1) {
    struct tm* current_time = ds3231_time_get(rtc);
    RTC_timeinfo = *current_time;
    free(current_time);
    temp = ds3231_temperature_get(rtc);
    
    printf("%04d-%02d-%02d %02d:%02d:%02d Temp: %.2f\n", RTC_timeinfo.tm_year + 1900 /*Add 1900 for better readability*/, RTC_timeinfo.tm_mon + 1,
            RTC_timeinfo.tm_mday, RTC_timeinfo.tm_hour, RTC_timeinfo.tm_min, RTC_timeinfo.tm_sec, temp);
    vTaskDelay(pdMS_TO_TICKS(60000));
  }
}

void display_task(void* pvParameters) {
  while(1) {
    
    int hour = RTC_timeinfo.tm_hour;
    int minute = RTC_timeinfo.tm_min;

    int display_value = hour * 100 + minute;

    tm1637_set_number(led, display_value, true, 0x04); // colon ON
    vTaskDelay(pdMS_TO_TICKS(500));

    tm1637_set_number(led, display_value, true, 0);    // colon OFF
    vTaskDelay(pdMS_TO_TICKS(500));
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

// Создание очередей
  queue_SNTP_to_RTC_data = xQueueCreate(5, sizeof(struct tm));

// Инициализация TM1637
  led = tm1637_init(GPIO_NUM_18, GPIO_NUM_19);
  tm1637_set_brightness(led, 3);

// Создание задачи Wi-Fi
  xTaskCreate(wifi_task, "wifi_task", 4096, NULL, 10, NULL);
  
}