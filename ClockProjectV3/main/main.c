#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include <time.h>
#include "driver/gpio.h"
#include "drivers/spi_master.h"

#include "esp_err.h"
#include "lvgl.h"
#include "st7735s.h"
#include "esp-idf-ds3231.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "lwip/apps/sntp.h"

#include "globals.h"
#include "tasks.h"
#include "ui.h"



void app_main(void) {
    lvgl_mutex = xSemaphoreCreateMutex();
    toDisplay_Queue = xQueueCreate(10, sizeof(struct toDisplay_data));
    SNTP_to_RTC_Queue = xQueueCreate(5, sizeof(struct tm));

    // For Events

    ESP_ERROR_CHECK(esp_event_loop_create_default());
  
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &my_wifi_connected_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &my_wifi_started_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &my_wifi_disconnected_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &my_wifi_ip_handler, NULL));

    // For SPI init

    spi_bus_config_t bus_config = {
        .mosi_io_num = MOSI_PIN,
        .miso_io_num = MISO_PIN,
        .sclk_io_num = SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
    };

    gpio_config_t io_conf = {};
    io_conf.pin_bit_mask = (1ULL << DC_PIN) | (1ULL << RST_PIN) | (1ULL << BKL_PIN);
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&io_conf);

    spi_device_interface_config_t device_config = {
        .command_bits = 0,          // Дисплеи ST7735S обычно не используют поле "команда" в SPI-протоколе
        .address_bits = 0,          // Аналогично, не используют поле "адрес"
        .dummy_bits = 0,            // Не используют "фиктивные" биты
        .mode = 0,                  // Режим SPI: CPOL=0, CPHA=0 (может быть 3 в некоторых случаях)
        .duty_cycle_pos = 128,      // 50% рабочий цикл (по умолчанию)
        .cs_ena_pretrans = 0,       // Задержка перед активацией CS (не нужна)
        .cs_ena_posttrans = 0,      // Задержка после деактивации CS (не нужна)
        .clock_speed_hz = SPI_MASTER_FREQ_40M, // Частота SPI-мастера, например 40 МГц
        
        .input_delay_ns = 0,        // Задержка входных данных (MISO не используется)
        .spics_io_num = CS_PIN, // Пин Chip Select (CS)
        .flags = 0,                 // Дополнительные флаги (обычно не нужны)
                                    // Например: SPI_DEVICE_NO_DUMMY
        .queue_size = 7,            // Размер очереди транзакций. 7 - это хороший баланс.
        .pre_cb = st7735s_spi_pre_transfer_callback, // Указатель на колбэк перед транзакцией (ОЧЕНЬ ВАЖНО!)
        .post_cb = NULL,            // Колбэк после транзакции (обычно не нужен)
    };
    
    spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO);

    spi_bus_add_device(SPI3_HOST, &device_config, &display_spi_handle);

    // Now SPI is initialized

    // We need to create a display TASK below !!!   |
    //                                              |
    //                                             \|/
    
    xTaskCreate(display_task, "DISPLAY_TASK", 4092, NULL, 10, NULL);
}   