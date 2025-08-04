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
#include "esp_timer.h"

i2c_master_bus_handle_t i2c_bus_handle;

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

    // Используем синхронную передачу
    esp_err_t ret = spi_device_transmit(display_spi_handle, &t);
    if (ret != ESP_OK) {
        ESP_LOGE("ST7735", "Failed to send command: 0x%02X, error: %s", cmd_val, esp_err_to_name(ret));
    }
}

void my_st7735_send_color(const uint8_t *data_buf, uint32_t len_bytes) {
    if (len_bytes == 0 || data_buf == NULL) {
        return;
    }

    // Для больших передач разбиваем на части
    const uint32_t max_transfer_size = 4096; // Максимальный размер передачи
    uint32_t remaining = len_bytes;
    const uint8_t *current_ptr = data_buf;

    while (remaining > 0) {
        uint32_t transfer_size = (remaining > max_transfer_size) ? max_transfer_size : remaining;
        
        spi_transaction_t t;
        memset(&t, 0, sizeof(t));
        
        t.length = transfer_size * 8;    // Длина в битах
        t.tx_buffer = current_ptr;       // Указатель на буфер с данными пикселей
        t.user = (void*)1;               // D/C = DATA

        // Используем синхронную передачу
        esp_err_t ret = spi_device_transmit(display_spi_handle, &t);
        if (ret != ESP_OK) {
            ESP_LOGE("ST7735", "Failed to send data, error: %s", esp_err_to_name(ret));
            return;
        }

        remaining -= transfer_size;
        current_ptr += transfer_size;
    }
}

void st7735s_send_gamma_profile( const uint8_t *gamma_pos, size_t gamma_pos_len,
                                 const uint8_t *gamma_neg, size_t gamma_neg_len)
{
    my_st7735_send_cmd(0xE0);
    my_st7735_send_color(gamma_pos, gamma_pos_len);

    my_st7735_send_cmd(0xE1);
    my_st7735_send_color(gamma_neg, gamma_neg_len);
}

// Исправленная функция flush callback
void st7735_flush_cb(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
    // Проверяем границы области
    int32_t x1 = area->x1;
    int32_t y1 = area->y1;
    int32_t x2 = area->x2;
    int32_t y2 = area->y2;
    
    // Убеждаемся, что координаты в пределах дисплея
    if (x1 < 0) x1 = 0;
    if (y1 < 0) y1 = 0;
    if (x2 >= 160) x2 = 159;
    if (y2 >= 128) y2 = 127;
    
    // Set column address
    my_st7735_send_cmd(0x2A); // CASET
    uint8_t col_data[4] = {
        (x1 >> 8) & 0xFF, x1 & 0xFF,
        (x2 >> 8) & 0xFF, x2 & 0xFF
    };
    my_st7735_send_color(col_data, 4);

    // Set row address  
    my_st7735_send_cmd(0x2B); // RASET
    uint8_t row_data[4] = {
        (y1 >> 8) & 0xFF, y1 & 0xFF,
        (y2 >> 8) & 0xFF, y2 & 0xFF
    };
    my_st7735_send_color(row_data, 4);

    // Start memory write
    my_st7735_send_cmd(0x2C); // RAMWR
    
    // Calculate data size
    uint32_t size = (x2 - x1 + 1) * (y2 - y1 + 1) * 2; // 2 bytes per pixel for RGB565
    
    // Send pixel data
    my_st7735_send_color(px_map, size);
    
    // Tell LVGL that flushing is done
    lv_display_flush_ready(disp);
}

void st7735s_spi_pre_transfer_callback(spi_transaction_t *t) {
    int dc = (int)t->user; // Получаем значение, которое мы передали в t->user
    gpio_set_level(DC_PIN, dc);
}

// Исправленная функция инициализации ST7735
void st7735_init_sequence(void) {
    ESP_LOGI("ST7735", "Starting display initialization...");
    
    // Software reset
    my_st7735_send_cmd(0x01); // SWRESET
    vTaskDelay(pdMS_TO_TICKS(150));

    // Sleep out
    my_st7735_send_cmd(0x11); // SLPOUT
    vTaskDelay(pdMS_TO_TICKS(255));

    // Frame rate control - более консервативные настройки
    my_st7735_send_cmd(0xB1); // FRMCTR1
    uint8_t frmctr1[] = {0x05, 0x3C, 0x3C}; // Более медленная частота кадров
    my_st7735_send_color(frmctr1, 3);

    my_st7735_send_cmd(0xB2); // FRMCTR2
    uint8_t frmctr2[] = {0x05, 0x3C, 0x3C};
    my_st7735_send_color(frmctr2, 3);

    my_st7735_send_cmd(0xB3); // FRMCTR3
    uint8_t frmctr3[] = {0x05, 0x3C, 0x3C, 0x05, 0x3C, 0x3C};
    my_st7735_send_color(frmctr3, 6);

    // Display inversion control
    my_st7735_send_cmd(0xB4); // INVCTR
    uint8_t invctr[] = {0x03}; // Изменено с 0x07 на 0x03
    my_st7735_send_color(invctr, 1);

    // Power control - более стабильные настройки
    my_st7735_send_cmd(0xC0); // PWCTR1
    uint8_t pwctr1[] = {0x28, 0x08, 0x04}; // Более консервативные значения
    my_st7735_send_color(pwctr1, 3);

    my_st7735_send_cmd(0xC1); // PWCTR2
    uint8_t pwctr2[] = {0xC0}; // Изменено с 0xC5
    my_st7735_send_color(pwctr2, 1);

    my_st7735_send_cmd(0xC2); // PWCTR3
    uint8_t pwctr3[] = {0x0D, 0x00};
    my_st7735_send_color(pwctr3, 2);

    my_st7735_send_cmd(0xC3); // PWCTR4
    uint8_t pwctr4[] = {0x8D, 0x2A};
    my_st7735_send_color(pwctr4, 2);

    my_st7735_send_cmd(0xC4); // PWCTR5
    uint8_t pwctr5[] = {0x8D, 0xEE};
    my_st7735_send_color(pwctr5, 2);

    // VCOM control
    my_st7735_send_cmd(0xC5); // VMCTR1
    uint8_t vmctr1[] = {0x1A}; // Изменено с 0x0E на 0x1A
    my_st7735_send_color(vmctr1, 1);

    // Display inversion
    my_st7735_send_cmd(0x21); // INVON - включаем инверсию (может улучшить отображение)

    // Memory access control - важные настройки для правильной ориентации
    my_st7735_send_cmd(0x36); // MADCTL
    uint8_t madctl[] = {0x60}; // Изменено с 0xC8 - попробуем другую ориентацию
    my_st7735_send_color(madctl, 1);

    // Pixel format
    my_st7735_send_cmd(0x3A); // COLMOD
    uint8_t colmod[] = {0x05}; // 16-bit color (RGB565)
    my_st7735_send_color(colmod, 1);

    // Установим точные границы для 1.77" 160x128 дисплея
    // Column address set
    my_st7735_send_cmd(0x2A); // CASET
    uint8_t caset[] = {0x00, 0x00, 0x00, 0x9F}; // 0 to 159
    my_st7735_send_color(caset, 4);

    // Row address set
    my_st7735_send_cmd(0x2B); // RASET  
    uint8_t raset[] = {0x00, 0x00, 0x00, 0x7F}; // 0 to 127
    my_st7735_send_color(raset, 4);

    // Set gamma curves
    ESP_LOGI("ST7735", "Setting gamma curves...");
    st7735s_send_gamma_profile(GAMMA_P_ARRAY, GAMMA_P_ARRAY_LEN, 
                              GAMMA_N_ARRAY, GAMMA_N_ARRAY_LEN);

    // Normal display on
    my_st7735_send_cmd(0x13); // NORON
    vTaskDelay(pdMS_TO_TICKS(10));

    // Display on
    my_st7735_send_cmd(0x29); // DISPON
    vTaskDelay(pdMS_TO_TICKS(100));
    
    // Очистим экран
    my_st7735_send_cmd(0x2C); // RAMWR
    
    ESP_LOGI("ST7735", "Display initialization completed successfully!");
}

// In globals.c, replace the global code with a function:

void init_display_pins(void) {
    ESP_LOGI("DISPLAY_INIT", "Initializing display pins...");
    
    // Configure RST pin
    gpio_config_t rst_config = {
        .pin_bit_mask = (1ULL << RST_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&rst_config));
    
    // Configure DC pin
    gpio_config_t dc_config = {
        .pin_bit_mask = (1ULL << DC_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    ESP_ERROR_CHECK(gpio_config(&dc_config));
    
    ESP_LOGI("DISPLAY_INIT", "GPIO pins configured successfully");
    
    // Reset sequence (выполняется один раз при инициализации дисплея)
    ESP_LOGI("DISPLAY_INIT", "Performing display reset sequence...");
    
    gpio_set_level(RST_PIN, 0); // Active low reset
    vTaskDelay(pdMS_TO_TICKS(100)); // Hold low for a bit
    gpio_set_level(RST_PIN, 1); // Release reset
    vTaskDelay(pdMS_TO_TICKS(100)); // Wait for display to come up
    
    ESP_LOGI("DISPLAY_INIT", "Display reset sequence completed");
}

// Исправленная функция create_splash_screen
void create_splash_screen(void) {
    ESP_LOGI(TAG_SPLASH, "Creating splash screen...");
    lv_obj_t *splash_screen;

    if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
        // 1. Создаем новый экран LVGL для приветствия
        splash_screen = lv_obj_create(NULL);
        if (splash_screen == NULL) {
            ESP_LOGE(TAG_SPLASH, "Failed to create splash screen object!");
            xSemaphoreGive(lvgl_mutex); 
            return;
        }
        lv_obj_set_style_bg_color(splash_screen, lv_color_black(), LV_PART_MAIN);
        
        // 2. Добавляем элементы на приветственный экран
        lv_obj_t *label = lv_label_create(splash_screen);
        if (label == NULL) {
            ESP_LOGE(TAG_SPLASH, "Failed to create label object!");
            lv_obj_del(splash_screen);
            xSemaphoreGive(lvgl_mutex);
            return;
        }
        lv_label_set_text(label, "Загрузка...");
        lv_obj_set_style_text_color(label, lv_color_white(), LV_PART_MAIN);
        lv_obj_center(label);
        
        // 3. Устанавливаем этот экран как активный
        lv_scr_load(splash_screen);
        
        xSemaphoreGive(lvgl_mutex);
    } else {
        ESP_LOGE(TAG_SPLASH, "Failed to take LVGL mutex for splash screen creation!");
        return;
    }

    ESP_LOGI(TAG_SPLASH, "Splash screen displayed.");

    // ВАЖНО: Даем время LVGL отрендерить экран
    for (int i = 0; i < 60; i++) { // 3 секунды с обновлениями LVGL
        vTaskDelay(pdMS_TO_TICKS(50));
        if (xSemaphoreTake(lvgl_mutex, pdMS_TO_TICKS(10)) == pdTRUE) {
            lv_timer_handler();
            xSemaphoreGive(lvgl_mutex);
        }
    }

    // Удаляем splash screen
    if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
        lv_obj_del(splash_screen);
        xSemaphoreGive(lvgl_mutex);
    }
    
    ESP_LOGI(TAG_SPLASH, "Splash screen removed.");
}


void create_main_screen(void) {
    ESP_LOGI("MAIN_SCREEN", "Creating main screen...");
    
    if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
        main_screen = lv_obj_create(NULL);

        if (main_screen == NULL) {
            ESP_LOGE("MAIN_SCREEN", "Failed to create main screen object!");
            xSemaphoreGive(lvgl_mutex);
            return;
        }

        lv_obj_set_style_bg_color(main_screen, lv_color_black(), LV_PART_MAIN);

        // For clock label
        lv_style_init(&clock_label_style);
        lv_style_set_text_font(&clock_label_style, LV_FONT_DEFAULT);

        clock_label = lv_label_create(main_screen);
        if (clock_label == NULL) {
            ESP_LOGE("MAIN_SCREEN", "Failed to create clock label!");
            lv_obj_del(main_screen);
            xSemaphoreGive(lvgl_mutex);
            return;
        }
        
        lv_label_set_text(clock_label, "00:00");
        lv_obj_set_style_text_color(clock_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_add_style(clock_label, &clock_label_style, 0);
        lv_obj_align(clock_label, LV_ALIGN_CENTER, 0, 0);

        // For temperature label
        lv_style_init(&temp_label_style);
        lv_style_set_text_font(&temp_label_style, LV_FONT_DEFAULT);

        temp_label = lv_label_create(main_screen);
        if (temp_label == NULL) {
            ESP_LOGE("MAIN_SCREEN", "Failed to create temperature label!");
            lv_obj_del(clock_label);
            lv_obj_del(main_screen);
            xSemaphoreGive(lvgl_mutex);
            return;
        }
        
        lv_label_set_text(temp_label, "0 °C");
        lv_obj_set_style_text_color(temp_label, lv_color_white(), LV_PART_MAIN);
        lv_obj_add_style(temp_label, &temp_label_style, 0);
        lv_obj_align(temp_label, LV_ALIGN_TOP_RIGHT, -10, 10);
        
        // Load the main screen
        lv_scr_load(main_screen);
        
        xSemaphoreGive(lvgl_mutex);
    } else {
        ESP_LOGE("MAIN_SCREEN", "Failed to take LVGL mutex for main screen creation!");
        return;
    }
    
    ESP_LOGI("MAIN_SCREEN", "Main screen created and loaded successfully!");
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

// Исправленная функция sntp_task с более частыми обновлениями
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
        ESP_LOGI("sntp_task", "Successfully sent the first SNTP data to SNTP_to_RTC_Queue...");
        xTaskCreate(RTC_Task, "RTC task", 4096, NULL, 6, NULL);
    } else {
        ESP_LOGW("sntp_task", "Failed to synchronize time");
        // Даже если синхронизация не удалась, запустим RTC с текущим временем
        xTaskCreate(RTC_Task, "RTC task", 4096, NULL, 6, NULL);
    }

    // Синхронизируем время каждые 10 минут вместо часа для лучшей точности
    while(1) {
        vTaskDelay(pdMS_TO_TICKS(600000));  // 600000 миллисекунд = 10 минут

        time_t now;
        struct tm timeinfo_updated;
        time(&now);
        localtime_r(&now, &timeinfo_updated); 
        
        // Неблокирующая отправка - если очередь заполнена, пропускаем
        if (xQueueSend(SNTP_to_RTC_Queue, &timeinfo_updated, 0) == pdTRUE) {
            ESP_LOGI("sntp_task", "Successfully sent updated SNTP data to SNTP_to_RTC_Queue...");
        } else {
            ESP_LOGW("sntp_task", "SNTP_to_RTC_Queue is full, skipping update");
        }
    }
}


// Исправленная функция RTC_Task
void RTC_Task(void* pvParameters) {
    // Allocate memory for the pointer of i2c_master_bus_handle_t
    i2c_master_bus_handle_t* i2c_bus = (i2c_master_bus_handle_t*)malloc(sizeof(i2c_master_bus_handle_t));

    i2c_master_bus_config_t i2c_bus_config = {
        .clk_source = I2C_CLK_SRC_DEFAULT,
        .i2c_port = -1,
        .scl_io_num = SCL_PIN,
        .sda_io_num = SDA_PIN,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, i2c_bus));

    rtc_handle_t *rtc_handle = ds3231_init(i2c_bus);

    if (rtc_handle == NULL) {
        ESP_LOGE("RTC_Task", "Failed to initialize DS3231");
        vTaskDelete(NULL);
        return;
    }

    // Ждем первоначальную синхронизацию времени от SNTP
    struct tm RTC_timeinfo;
    if (xQueueReceive(SNTP_to_RTC_Queue, &RTC_timeinfo, portMAX_DELAY) == pdTRUE) {
        ESP_LOGI("RTC_Task", "Initial RTC update from SNTP...");
        ESP_ERROR_CHECK(ds3231_time_tm_set(rtc_handle, RTC_timeinfo));
    }

    // Основной цикл - читаем RTC каждую секунду
    while(1) {
        // Проверяем, есть ли обновления от SNTP (неблокирующий вызов)
        if (xQueueReceive(SNTP_to_RTC_Queue, &RTC_timeinfo, 0) == pdTRUE) {
            ESP_LOGI("RTC_Task", "Updating RTC from SNTP...");
            ESP_ERROR_CHECK(ds3231_time_tm_set(rtc_handle, RTC_timeinfo));
        }

        // Читаем текущее время и температуру с RTC
        struct tm* current_time = ds3231_time_get(rtc_handle);
        if (current_time != NULL) {
            RTC_timeinfo = *current_time;
            free(current_time);
            
            float temp = ds3231_temperature_get(rtc_handle);
            
            struct toDisplay_data toDisplay = {RTC_timeinfo.tm_hour, RTC_timeinfo.tm_min, temp};
            
            // Отправляем данные на дисплей (неблокирующий вызов)
            if (xQueueSend(toDisplay_Queue, &toDisplay, 0) == pdTRUE) {
                ESP_LOGI("RTC_TASK", "Sent to DISPLAY_TASK: %02d:%02d, %.1f°C", 
                         toDisplay.hour, toDisplay.min, toDisplay.temp);
            } else {
                ESP_LOGW("RTC_TASK", "Failed to send to display queue (queue full)");
            }
        } else {
            ESP_LOGE("RTC_Task", "Failed to read time from RTC");
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000)); // Обновляем каждую секунду
    }
}

// Исправленная функция display_task
void display_task(void* pvParameters) {
    struct toDisplay_data from_RTC; 
    
    ESP_LOGI("DISPLAY_TASK", "Starting display task...");
    
    init_display_pins();

    lv_init();
    lv_tick_set_cb(lv_tick_cb);
    
    // Create display using universal function
    lv_display_t *display = lv_display_create(160, 128);
    
    if (display == NULL) {
        ESP_LOGE("LVGL_INIT", "Failed to create LVGL display!");
        return;
    }

    // Увеличенный буфер для лучшего качества
    static uint8_t buf1[160 * 20 * 2]; // Буфер для 20 строк
    static uint8_t buf2[160 * 20 * 2]; // Двойной буфер
    lv_display_set_buffers(display, buf1, buf2, sizeof(buf1), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Set flush callback for display
    lv_display_set_flush_cb(display, st7735_flush_cb);

    // Set color format
    lv_display_set_color_format(display, LV_COLOR_FORMAT_RGB565);

    // Initialize ST7735 display manually
    st7735_init_sequence();

    ESP_LOGI("DISPLAY_TASK", "Starting background tasks...");
    
    // Запускаем LVGL task ДО создания UI
    xTaskCreate(lvgl_main_task, "LVGL_TIMER_HANDLER", 4096, NULL, 10, NULL);
    
    // Даем время LVGL инициализироваться
    vTaskDelay(pdMS_TO_TICKS(100));
    
    ESP_LOGI("DISPLAY_TASK", "Creating UI screens...");
    
    // Create and show splash screen first
    create_splash_screen();
    
    // Create main screen (it will be loaded automatically)
    create_main_screen();

    // Запускаем Wi-Fi после создания UI
    xTaskCreate(wifi_task, "WIFI_TASK", 4096, NULL, 5, NULL);
    
    ESP_LOGI("DISPLAY_TASK", "Display task initialized, entering main loop...");
    
    while(1) {
        if (xQueueReceive(toDisplay_Queue, &from_RTC, portMAX_DELAY) == pdTRUE) {
            ESP_LOGI("DISPLAY_TASK", "Received data from RTC_task: %02d:%02d, %.1f°C", 
                     from_RTC.hour, from_RTC.min, from_RTC.temp);
            
            if (xSemaphoreTake(lvgl_mutex, portMAX_DELAY) == pdTRUE) {
                if (clock_label != NULL && temp_label != NULL) {
                    lv_label_set_text_fmt(clock_label, "%02d:%02d", from_RTC.hour, from_RTC.min);
                    lv_label_set_text_fmt(temp_label, "%.1f °C", from_RTC.temp);
                    lv_obj_invalidate(clock_label);
                    lv_obj_invalidate(temp_label);
                } else {
                    ESP_LOGW("DISPLAY_TASK", "Labels are not initialized yet");
                }
                xSemaphoreGive(lvgl_mutex);
            }
        }
    }
}