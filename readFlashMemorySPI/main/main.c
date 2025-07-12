#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include <stdio.h>
#include <inttypes.h>
#include "driver/gpio.h"
#include "driver/spi_master.h"
#include "driver/uart.h"

#define PIN_MOSI GPIO_NUM_23
#define PIN_MISO GPIO_NUM_19
#define PIN_SCK GPIO_NUM_18
#define PIN_CS GPIO_NUM_5

#define FLASH_SIZE_BYTES (2 * 1024 * 1024)  // 2 мегабайта
#define CHUNK_SIZE 256  

void flash_read_all(spi_device_handle_t spi) {
    uint8_t buf[CHUNK_SIZE];

    // Установка настроек для полудуплексного чтения
    // Обратите внимание на .length и .rxlength
    spi_transaction_t t = {
        .cmd = 0x03,              // Команда READ
        .addr = 0x000000,         // Адрес будет обновляться в цикле
        .length = 0,              // Длина данных для передачи в байтах (0, так как отправляем только команду и адрес)
        .rxlength = CHUNK_SIZE * 8, // Длина данных для приема в битах
        .rx_buffer = buf,
    };

    printf("START_DUMP\n"); // Отмечаем начало дампа

    for (uint32_t addr = 0; addr < FLASH_SIZE_BYTES; addr += CHUNK_SIZE) {
        t.addr = addr;
        esp_err_t ret = spi_device_transmit(spi, &t);
        if (ret != ESP_OK) {
            printf("SPI read error at 0x%06" PRIX32 ": %s\n", addr, esp_err_to_name(ret));
            break;
        }
        
        // Вместо printf() отправляем сырые байты по UART0
        uart_write_bytes(UART_NUM_0, (const char*)buf, CHUNK_SIZE);
    }
    printf("\nEND_DUMP\n"); // Отмечаем конец дампа
}


void reading_data_Task(void *pvParameters) {
    const spi_host_device_t HOST_ID = SPI3_HOST;
    spi_device_handle_t spi_flash;

    spi_bus_config_t bus_config = {
        .mosi_io_num = PIN_MOSI,
        .miso_io_num = PIN_MISO,
        .sclk_io_num = PIN_SCK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4092,
        .intr_flags = 0,
        .flags = 0,
    };

    ESP_ERROR_CHECK(spi_bus_initialize(HOST_ID, &bus_config, SPI_DMA_CH_AUTO));

spi_device_interface_config_t dev_config = {
    .command_bits = 8,                  // Обычно команды SPI Flash — 1 байт
    .address_bits = 24,                 // Winbond W25Q16 имеет 24-битную адресацию
    .dummy_bits = 0,                    // Для Fast Read требуется 8 "dummy" бит (настраивается при чтении)
    .mode = 0,                          // SPI Mode 0 (CPOL=0, CPHA=0) — стандарт для Winbond
    .clock_speed_hz = 20 * 1000 * 1000, // 20 MHz — безопасная частота для большинства схем
    .spics_io_num = PIN_CS,             // Укажи здесь номер GPIO для Chip Select
    .queue_size = 1,                    // Очередь команд (можно увеличить при необходимости)
    .flags = SPI_DEVICE_HALFDUPLEX,     // Flash обычно работает в half-duplex
    .pre_cb = NULL,
};


    ESP_ERROR_CHECK(spi_bus_add_device(HOST_ID, &dev_config, &spi_flash));

    flash_read_all(spi_flash);

    // Освобождаем ресурсы SPI после завершения работы
    spi_bus_remove_device(spi_flash);
    spi_bus_free(HOST_ID);

    // Удаляем задачу после завершения
    vTaskDelete(NULL);
}

void app_main(void) {
    // ВАЖНО: Настройка и установка драйвера UART до создания задачи
    // Эта часть кода гарантирует, что UART будет работать с буферами.
    uart_config_t uart_config = {
        .baud_rate = 115200, 
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_0, &uart_config));
    // Устанавливаем драйвер с буфером для передачи
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_0, 256, 0, 0, NULL, 0));

    xTaskCreate(reading_data_Task, "flash_dump", 8192, NULL, 5, NULL);
    
}