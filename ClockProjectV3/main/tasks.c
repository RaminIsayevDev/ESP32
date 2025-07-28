#include "tasks.h"
#include "esp_err.h"
#include "lvgl.h"
#include "st7735s.h"
#include "globals.h"
#include "esp-idf-ds3231.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/apps/sntp.h"
#include "driver/gpio.h"

// Handlers

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

            //  Sending to Display via toDisplayQueue

            DisplayMessage msg;
            

            msg.type = DISPLAY_WIFI_STATUS;
            msg.data.wifi_status.connected = true;
            strcpy(msg.data.wifi_status.ssid, "ALHN-3405");

            xQueueSend(toDisplay_Queue, &msg, portMAX_DELAY);

            ESP_LOGI(TAG_IP, "Succesfully sent wifi_status to toDisplay_Queue");

            // Start sntp_task

            sntp_started = true;
            xTaskCreate(sntp_task, "sntp_task", 4096, NULL, 3, NULL);
        }
    } else {
        printf("Can't receive the IP...");
    }
}

// Tasks

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
        xQueueSend(SNTP_to_RTC_Queue, &timeinfo, portMAX_DELAY);
        ESP_LOGI("sntp_task", "Succesfully sent the first SNTP data to SNTP_to_RTC_Queue...");
        xTaskCreate(RTC_Task, "RTC task", 2048, NULL, 4, NULL);
    } else {
        ESP_LOGW("sntp_task", "Failed to synchronize time");
    }

    while(1) {
        vTaskDelay(pdMS_TO_TICKS(3600000));  // 3600000 миллисекунд = 1 час

        time_t now;
        struct tm timeinfo_updated;
        time(&now);
        localtime_r(&now, &timeinfo_updated); 
        xQueueSend(SNTP_to_RTC_Queue, &timeinfo_updated, portMAX_DELAY); 

        // Free up memory before sleeping task

        free(&now);
        free(&timeinfo_updated);

        ESP_LOGI("sntp_task", "Successfully sent updated SNTP data to SNTP_to_RTC_Queue...");
    }
}

void RTC_Task(void* pvParameters) {

  i2c_master_dev_handle_t* rtc_handle = ds3231_init_full(GPIO_NUM_21, GPIO_NUM_22);

  if (rtc_handle == NULL) {
      ESP_LOGE("RTC_Task", "Failed to initialize DS3231");
      vTaskDelete(NULL); // Завершить задачу, если инициализация не удалась
  }

  while(1) {
    struct tm RTC_timeinfo;
    float temp;

    

    if (xQueueReceive(SNTP_to_RTC_Queue, &RTC_timeinfo, portMAX_DELAY) == pdTRUE) {
      ESP_LOGI("RTC_Task", "RTC updating from SNTP...");
      ESP_ERROR_CHECK(ds3231_time_tm_set(rtc_handle, RTC_timeinfo));
    }

    struct tm* current_time = ds3231_time_get(rtc_handle);
    RTC_timeinfo = *current_time;
    free(current_time);
    temp = ds3231_temperature_get(rtc_handle);
    
    printf("%04d-%02d-%02d %02d:%02d:%02d Temp: %.1f\n", RTC_timeinfo.tm_year + 1900 /*Add 1900 for better readability*/, RTC_timeinfo.tm_mon + 1,
            RTC_timeinfo.tm_mday, RTC_timeinfo.tm_hour, RTC_timeinfo.tm_min, RTC_timeinfo.tm_sec, temp);

    DisplayMessage msg;
    msg.type = DISPLAY_CLOCK_TEMP;
    snprintf(msg.data.clock_temp.time, sizeof(msg.data.clock_temp.time), 
         "%02d:%02d:%02d", 
         RTC_timeinfo.tm_hour, 
         RTC_timeinfo.tm_min, 
         RTC_timeinfo.tm_sec);
    snprintf(msg.data.clock_temp.temp, sizeof(msg.data.clock_temp.temp), 
         "%.1f C", 
         temp);

    xQueueSend(toDisplay_Queue, &msg, portMAX_DELAY);
    ESP_LOGI("RTC_Task", "Succesfully sent to toDisplay_Queue...");
    ESP_LOGI("RTC_Task", "Set clock: %s, temp: %s", msg.data.clock_temp.time, msg.data.clock_temp.temp);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void display_task(void* pvParameters) {
    DisplayMessage msg;
    struct tm timeinfo;

    lv_obj_t *screen = lv_scr_act();  // Get current screen

    lv_obj_t* wifi_label = lv_label_create(screen);
    lv_obj_align(wifi_label, LV_ALIGN_TOP_LEFT, 10, 10);

    lv_obj_t* ssid_label = lv_label_create(screen);
    lv_obj_align(ssid_label, LV_ALIGN_TOP_LEFT, 10, 30);

    lv_obj_t* clock_label = lv_label_create(screen);
    lv_obj_align(clock_label, LV_ALIGN_TOP_LEFT, 10, 60);

    lv_obj_t* temp_label = lv_label_create(screen);
    lv_obj_align(temp_label, LV_ALIGN_TOP_LEFT, 10, 80);

    lv_scr_load(screen);

    while(1) {
        if (xQueueReceive(toDisplay_Queue, &msg, portMAX_DELAY)) {
            switch (msg.type) {
                case DISPLAY_WIFI_STATUS:
                    if (msg.data.wifi_status.connected) {
                        lv_label_set_text(wifi_label, "WiFi: Connected");
                        lv_label_set_text(ssid_label, msg.data.wifi_status.ssid);
                    } else {
                        lv_label_set_text(wifi_label, "WiFi: Connecting...");
                    }
                    break;

                case DISPLAY_CLOCK_TEMP:
                    ESP_LOGI("Display_Task", "Set clock: %s, temp: %s", msg.data.clock_temp.time, msg.data.clock_temp.temp); // For log
                    lv_label_set_text(clock_label, msg.data.clock_temp.time);
                    lv_label_set_text(temp_label, msg.data.clock_temp.temp);
                    break;
            }
        }
    
      lv_timer_handler();  // Process LVGL tasks
      vTaskDelay(pdMS_TO_TICKS(5));  // Call periodically
    }
}