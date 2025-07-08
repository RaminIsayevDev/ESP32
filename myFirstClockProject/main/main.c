#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_sntp.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "time.h"

// --- НАСТРОЙКИ (ИЗМЕНИТЕ ЭТИ ЗНАЧЕНИЯ) ---
#define WIFI_SSID      "ALHN-3405"
#define WIFI_PASSWORD  "0554540509"

// GPIO для 7-сегментного дисплея (Общий Катод)
// Сегменты (a, b, c, d, e, f, g, dp)
const int segment_pins[] = {25, 26, 27, 14, 12, 13, 15, 2}; 
// Пины выбора разряда (D1, D2, D3, D4)
const int digit_pins[] = {33, 32, 21, 19}; 

// --- ГЛОБАЛЬНЫЕ ПЕРЕМЕННЫЕ ---
static const char *TAG_WIFI = "WIFI_TASK";
static const char *TAG_TIME = "TIME_TASK";
static const char *TAG_DISP = "DISPLAY_TASK";

// Объявление очередей
QueueHandle_t queue1_time_struct;
QueueHandle_t queue2_display_digits;

// Структура для передачи цифр на дисплей
typedef struct {
    uint8_t digits[4]; // [H1, H2, M1, M2]
} display_data_t;

// Карта сегментов для цифр от 0 до 9 (для общего катода)
// G-F-E-D-C-B-A
const uint8_t segment_map[] = {
    0b00111111, // 0
    0b00000110, // 1
    0b01011011, // 2
    0b01001111, // 3
    0b01100110, // 4
    0b01101101, // 5
    0b01111101, // 6
    0b00000111, // 7
    0b01111111, // 8
    0b01101111, // 9
};

// --- ЗАДАЧА 1: WIFI и SNTP ---
void wifi_sntp_task(void *pvParameters) {
    // 1. Инициализация и подключение к WiFi
    ESP_LOGI(TAG_WIFI, "Connecting to WiFi...");
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&cfg);
    
    wifi_config_t wifi_config = {
        .sta = { .ssid = WIFI_SSID, .password = WIFI_PASSWORD },
    };
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config);
    esp_wifi_start();
    esp_wifi_connect();
    
    // Ожидание подключения (простой вариант)
    // В реальном проекте лучше использовать Event Group Bits
    int retries = 0;
    while(sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET && ++retries < 30) {
        ESP_LOGI(TAG_WIFI, "Waiting for WiFi connection... (%d/30)", retries);
        vTaskDelay(pdMS_TO_TICKS(2000));
    }

    // 2. Инициализация SNTP
    ESP_LOGI(TAG_WIFI, "Initializing SNTP");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_init();

    // 3. Главный цикл задачи
    while (1) {
        // Ожидание синхронизации времени
        while (sntp_get_sync_status() == SNTP_SYNC_STATUS_RESET) {
            ESP_LOGI(TAG_WIFI, "Waiting for time synchronization...");
            vTaskDelay(pdMS_TO_TICKS(5000));
        }

        // Время синхронизировано, получаем его
        time_t now;
        struct tm timeinfo;
        time(&now);
        // Устанавливаем часовой пояс (например, UTC+4 для Баку)
        setenv("TZ", "AZT-4", 1);
        tzset();
        localtime_r(&now, &timeinfo);
        
        ESP_LOGI(TAG_WIFI, "Time is synchronized: %02d:%02d:%02d", timeinfo.tm_hour, timeinfo.tm_min, timeinfo.tm_sec);
        
        // Отправка структуры времени в Queue 1
        if (xQueueSend(queue1_time_struct, &timeinfo, pdMS_TO_TICKS(1000)) == pdPASS) {
            ESP_LOGI(TAG_WIFI, "Time structure sent to Queue 1.");
        } else {
            ESP_LOGW(TAG_WIFI, "Failed to send to Queue 1.");
        }

        // Засыпаем на 1 час перед следующей синхронизацией
        ESP_LOGI(TAG_WIFI, "Task sleeping for 1 hour.");
        vTaskDelay(pdMS_TO_TICKS(3600 * 1000)); 
    }
}

// --- ЗАДАЧА 2: ОБРАБОТКА ВРЕМЕНИ ---
void time_processing_task(void *pvParameters) {
    struct tm received_time;
    display_data_t data_to_display;

    while (1) {
        // Ожидание данных из Queue 1 (блокирующее)
        if (xQueueReceive(queue1_time_struct, &received_time, portMAX_DELAY) == pdPASS) {
            ESP_LOGI(TAG_TIME, "Received time from Queue 1: %02d:%02d", received_time.tm_hour, received_time.tm_min);

            // Разложение часов и минут на цифры
            data_to_display.digits[0] = received_time.tm_hour / 10;
            data_to_display.digits[1] = received_time.tm_hour % 10;
            data_to_display.digits[2] = received_time.tm_min / 10;
            data_to_display.digits[3] = received_time.tm_min % 10;

            // Отправка массива цифр в Queue 2
            if (xQueueSend(queue2_display_digits, &data_to_display, pdMS_TO_TICKS(1000)) == pdPASS) {
                ESP_LOGI(TAG_TIME, "Digits [%d,%d,%d,%d] sent to Queue 2.", 
                    data_to_display.digits[0], data_to_display.digits[1], 
                    data_to_display.digits[2], data_to_display.digits[3]);
            } else {
                 ESP_LOGW(TAG_TIME, "Failed to send to Queue 2.");
            }
        }
    }
}

// Вспомогательная функция для зажигания одной цифры на дисплее (ОБЩИЙ КАТОД - ИСПРАВЛЕНО)
void display_digit(int digit_pos, int number) {
    // Выключаем все разряды, подавая на них высокий уровень (HIGH)
    for (int i = 0; i < 4; i++) {
        gpio_set_level(digit_pins[i], 1); 
    }

    // Устанавливаем сегменты для нужной цифры (подаем HIGH на нужные сегменты)
    // Эта часть была правильной
    uint8_t segments = segment_map[number];
    for (int i = 0; i < 8; i++) {
        // Устанавливаем 0 на все сегменты перед отрисовкой новой цифры
        gpio_set_level(segment_pins[i], 0);
        if ((segments >> i) & 1) {
            gpio_set_level(segment_pins[i], 1);
        }
    }
    
    // Включаем нужный разряд, подавая на него низкий уровень (LOW)
    gpio_set_level(digit_pins[digit_pos], 0); 
}

// --- ЗАДАЧА 3: ОТОБРАЖЕНИЕ НА 7-СЕГМЕНТНОМ ИНДИКАТОРЕ ---
// --- ЗАДАЧА 3: ОТОБРАЖЕНИЕ (ИСПРАВЛЕННАЯ ВЕРСИЯ) ---
void display_task(void *pvParameters) {
    // ... (код инициализации GPIO) ...

    display_data_t current_display_data;
    memset(&current_display_data, 0, sizeof(display_data_t));
    current_display_data.digits[0] = 8;
    current_display_data.digits[1] = 8;
    current_display_data.digits[2] = 8;
    current_display_data.digits[3] = 8;


    while (1) {
        // Проверка Queue 2 на наличие новых данных (неблокирующая)
        if (xQueueReceive(queue2_display_digits, &current_display_data, 0) == pdPASS) {
             ESP_LOGI(TAG_DISP, "New display data received. Colon: %d", current_display_data.show_colon);
        }

        // Мультиплексирование дисплея
        for (int i = 0; i < 4; i++) {
            display_digit(i, current_display_data.digits[i], current_display_data.show_colon);
            
            // !!! ВОТ КЛЮЧЕВАЯ СТРОКА !!!
            // Эта функция создает паузу в 5 миллисекунд и, что самое главное,
            // отдает управление планировщику FreeRTOS, чтобы могли работать другие задачи.
            vTaskDelay(pdMS_TO_TICKS(5)); 
        }
    }
}


// --- ГЛАВНАЯ ФУНКЦИЯ ---
void app_main(void) {
    // Инициализация NVS (необходима для работы WiFi)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Создание очередей
    // Queue 1: для передачи полной структуры времени (tm)
    queue1_time_struct = xQueueCreate(1, sizeof(struct tm));
    // Queue 2: для передачи 4 цифр на дисплей
    queue2_display_digits = xQueueCreate(1, sizeof(display_data_t));
    
    if (queue1_time_struct == NULL || queue2_display_digits == NULL) {
        ESP_LOGE("MAIN", "Failed to create queues.");
        return;
    }

    // Создание задач
    xTaskCreate(wifi_sntp_task, "WiFi SNTP Task", 4096, NULL, 5, NULL);
    xTaskCreate(time_processing_task, "Time Processing Task", 2048, NULL, 5, NULL);
    xTaskCreate(display_task, "Display Task", 2048, NULL, 4, NULL); // Приоритет чуть ниже, но не критично
    
    ESP_LOGI("MAIN", "Initialization complete. Tasks started.");
}