#include "tasks.h"
#include "esp_err.h"
#include "lvgl.h"
#include "globals.h"
#include "esp-idf-ds3231.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/apps/sntp.h"
#include "driver/gpio.h"
#include "driver/i2c_master.h"
#include "ui_helpers.h"
#include "ui_Screen1.h"
#include "ui_comp.h"
#include "esp_timer.h"

// functions 

uint32_t lv_tick_cb(void)
{
    return esp_timer_get_time() / 1000;
}

void my_st7735_send_cmd(uint8_t cmd_val) {
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));       // Очистить структуру транзакции

    t.length = 8;                   // Команда всегда 8 бит
    t.tx_buffer = &cmd_val;         // Указатель на буфер с командой
    t.user = (void*)0;              // Передаем 0 в t.user -> pre_cb установит D/C в режим "команда" (0)

    // Отправляем транзакцию по SPI
    spi_device_transmit(display_spi_handle, &t);
}

void my_st7735_send_color(const uint8_t *data_buf, uint32_t len_bytes) {
    spi_transaction_t *t; 

    t = (spi_transaction_t *)heap_caps_malloc(sizeof(spi_transaction_t), MALLOC_CAP_DMA);
    if (t == NULL) {
        // Обработка ошибки, если не удалось выделить память
        ESP_LOGE("ST7735", "Failed to allocate SPI transaction");
        return;
    }
    memset(t, 0, sizeof(spi_transaction_t)); // Очистить структуру транзакции

    t->length = len_bytes * 8;       // Длина в битах
    t->tx_buffer = data_buf;         // Указатель на буфер с данными пикселей
    t->user = (void*)1;              // D/C = DATA

    // Запуск асинхронной передачи. Функция возвращает сразу.
    // Передача будет происходить в фоновом режиме через DMA.
    ESP_ERROR_CHECK(spi_device_queue_trans(display_spi_handle, t, portMAX_DELAY));
}

void st7735s_send_gamma_profile( const uint8_t *gamma_pos, size_t gamma_pos_len,
                                 const uint8_t *gamma_neg, size_t gamma_neg_len)
{
    my_st7735_send_cmd(0xE0);
    my_st7735_send_color(gamma_pos, gamma_pos_len);

    my_st7735_send_cmd(0xE1);
    my_st7735_send_color(gamma_neg, gamma_neg_len);
}

void st7735s_spi_pre_transfer_callback(spi_transaction_t *t) {
    int dc = (int)t->user; // Получаем значение, которое мы передали в t->user
    gpio_set_level(DC_PIN, dc);
}

void display_restart(void) {
  gpio_set_level(RST_PIN, 0); // Active low reset
  vTaskDelay(pdMS_TO_TICKS(100)); // Hold low for a bit
  gpio_set_level(RST_PIN, 1); // Release reset
  vTaskDelay(pdMS_TO_TICKS(100)); 
}

void create_splash_screen(void) {
  ESP_LOGI(TAG_SPLASH, "Creating splash screen...");

  if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
    // 1. Создаем новый экран LVGL для приветствия
    lv_obj_t *splash_screen = lv_obj_create(NULL); // NULL означает, что это будет новый экран
    if (splash_screen == NULL) {
        ESP_LOGE(TAG_SPLASH, "Failed to create splash screen object!");
        xSemaphoreGive(lvgl_mutex); // Освобождаем мьютекс при ошибке
        return;
    }
    lv_obj_set_style_bg_color(splash_screen, lv_color_black(), LV_PART_MAIN); // Черный фо
    // 2. Добавляем элементы на приветственный экра
    // Пример: Добавление текста
    lv_obj_t *label = lv_label_create(splash_screen);
    if (label == NULL) {
        ESP_LOGE(TAG_SPLASH, "Failed to create label object!");
        lv_obj_del(splash_screen); // Удаляем уже созданный экран
        xSemaphoreGive(lvgl_mutex); // Освобождаем мьютекс при ошибке
        return;
    }
    lv_label_set_text(label, "Загрузка...");
    lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
    lv_obj_center(label); // Выравниваем текст по центр
    // 3. Устанавливаем этот экран как активный
    lv_disp_load_scr(splash_screen); // Загружаем приветственный экран
    xSemaphoreGive(lvgl_mutex); // *** ОСВОБОЖДАЕМ МЬЮТЕКС ПЕРЕД БЛОКИРУЮЩЕЙ ЗАДЕРЖКОЙ ***
    } else {
      ESP_LOGE(TAG_SPLASH, "Failed to take LVGL mutex for splash screen creation!");
      return; // Выходим, если не удалось взять мьютекс
    }

  ESP_LOGI(TAG_SPLASH, "Splash screen displayed.");

  vTaskDelay(pdMS_TO_TICKS(3000)); // Задержка 3 секунды
}

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
            // Start sntp_task
            sntp_started = true;
            xTaskCreate(sntp_task, "sntp_task", 4096, NULL, 3, NULL);
        }
    } else {
        printf("Can't receive the IP...");
    }
}

// Tasks

void lvgl_main_task(void *pvParameter) {
  (void) pvParameter;
  while (1) {
    // Захватываем мьютекс для lv_timer_handler, так как он изменяет состояние LVGL
    if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
        lv_timer_handler(); // Обработка всех задач LVGL
        xSemaphoreGive(lvgl_mutex);
    }
    vTaskDelay(pdMS_TO_TICKS(50)); // Небольшая задержка, чтобы не загружать CPU
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
        xQueueSend(SNTP_to_RTC_Queue, &timeinfo, portMAX_DELAY);
        ESP_LOGI("sntp_task", "Succesfully sent the first SNTP data to SNTP_to_RTC_Queue...");
        xTaskCreate(RTC_Task, "RTC task", 4096, NULL, 6, NULL);
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

        ESP_LOGI("sntp_task", "Successfully sent updated SNTP data to SNTP_to_RTC_Queue...");
    }
}

void RTC_Task(void* pvParameters) {

  i2c_master_bus_config_t i2c_bus_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_NUM_0,
    .scl_io_num = SCL_PIN,
    .sda_io_num = SDA_PIN,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
  };

  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus));

  ESP_LOGI("RTC_Task", "I2C master bus initialized");

  rtc_handle_t *rtc_handle = ds3231_init(&i2c_bus);

  if (rtc_handle == NULL) {
      ESP_LOGE("RTC_Task", "Failed to initialize DS3231");
      vTaskDelete(NULL); // Завершить задачу, если инициализация не удалась
      return;
  }

  ESP_LOGI("RTC_Task", "DS3231 succesfully initialized");

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
    
    struct toDisplay_data toDisplay = {RTC_timeinfo.tm_hour, RTC_timeinfo.tm_min, temp};

    xQueueSend(toDisplay_Queue, &toDisplay, portMAX_DELAY);
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

void display_task(void* pvParameters) {
  display_restart();

  lv_init();
  lv_tick_set_cb(lv_tick_cb);
  
  lv_display_t *display = lv_st7735_create(160, 128, LV_LCD_FLAG_BGR_COLOR_MODE, my_st7735_send_cmd, my_st7735_send_color);

  if (display == NULL) {
    ESP_LOGE("LVGL_INIT", "Failed to create LVGL display!");
    return;
  }

  static uint8_t buf[160 * 128 / 10 * 2];
  lv_display_set_buffers(display, buf, NULL, LV_DISPLAY_RENDER_MODE_PARTIAL);



  // Now let's create UI

  create_splash_screen();

  while(1) {
    
  }
}