#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

    ret = spi_bus_initialize(SPI3_HOST, &bus_config, SPI_DMA_CH_AUTO);

    if (ret != ESP_OK) {
        printf("Can't initialize SPI bus...");
        return;
    }

    ret = spi_bus_add_device(SPI3_HOST, );

}