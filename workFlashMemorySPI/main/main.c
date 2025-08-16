#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "stdio.h"
#include "driver/gpio.h"
#include "driver/spi_master.h"

#define MOSI_PIN GPIO_NUM_23
#define MISO_PIN GPIO_NUM_19
#define SCLK_PIN GPIO_NUM_18
#define CS_PIN GPIO_NUM_5

void app_main(void) {
    esp_err_t ret;

    spi_bus_config_t bus_config = {
        .mosi_io_num = MOSI_PIN,
        .miso_io_num = MISO_PIN,
        .sclk_io_num = SCLK_PIN,
        .quadhd_io_num = -1,
        .quadwp_io_num = -1,
    };

    spi_device_interface_config_t device_config = {
        .address_bits = 24,
        .command_bits = 8,
        .mode = 0,
        .dummy_bits = 0,
        .clock_speed_hz = 20 * 1000 * 1000, // 20 MHz
        .spics_io_num = CS_PIN,
        .queue_size = 1,
        .flags = SPI_DEVICE_HALFDUPLEX,
        .pre_cb = NULL,
    };

    spi_device_handle_t flash_handle;

    ret = spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO);

    if (ret != ESP_OK) {
        printf("Can't initialize SPI bus...");
        return;
    }

    ret = spi_bus_add_device(SPI3_HOST, &device_config, &flash_handle);

    spi_transaction_t t;
    spi_transaction_t t_status;

    memset(&t, 0, sizeof(t));   // Clear transaction structure
    memset(&t_status, 0, sizeof(t_status));

    uint8_t tx_data[2] = {0x06, 0xC7};  // Data for send

    uint8_t tx_status_data[1] = {0x05};
    uint8_t rx_status_data;

    t.length = 8;
    t.tx_buffer = &tx_data;  // Pointer to transmit data
    t.rx_buffer = NULL;      // There is no MISO phase

    t_status.length = 8;
    t_status.tx_buffer = &tx_status_data;
    t_status.rx_buffer = &rx_status_data;

    ret = spi_device_transmit(flash_handle, &t);
    if (ret != ESP_OK) {
        printf("Can't transmit to device...");
        return;
    }

    do {
        vTaskDelay(pdMS_TO_TICKS(100));
        ret = spi_device_transmit(flash_handle, &t_status);

        if (ret != ESP_OK) {
            printf("Can't polling for status(transmit)...");
            return;
        }

    } while (rx_status_data & 0x01);

    printf("Chip erased succesfully!");
}