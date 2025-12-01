#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include <string.h>
#include <stdio.h>

#define MOSI_PIN GPIO_NUM_23
#define MISO_PIN GPIO_NUM_19
#define SCLK_PIN GPIO_NUM_18
#define CS_PIN   GPIO_NUM_4
#define WP_PIN   GPIO_NUM_22
#define HOLD_PIN GPIO_NUM_21

spi_device_handle_t flash;

// 1) Write Enable
esp_err_t flash_write_enable()
{
    spi_transaction_t t = {0};
    t.cmd = 0x06;
    return spi_device_transmit(flash, &t);
}

// 2) Read Status Register
uint8_t flash_read_status()
{
    spi_transaction_t t = {0};
    t.cmd = 0x05;
    t.length = 8;
    t.rxlength = 8;
    t.flags = SPI_TRANS_USE_RXDATA; 
    
    esp_err_t err = spi_device_transmit(flash, &t);
    if (err != ESP_OK) return 0xFF;

    return t.rx_data[0];
}

// NEW: Read JEDEC ID (0x9F) to verify wiring
uint32_t flash_read_id()
{
    spi_transaction_t t = {0};
    t.cmd = 0x9F;
    t.length = 24;   // Read 3 bytes (Manufacturer, Memory Type, Capacity)
    t.rxlength = 24;
    t.flags = SPI_TRANS_USE_RXDATA;
    
    esp_err_t err = spi_device_transmit(flash, &t);
    if (err != ESP_OK) return 0;

    uint32_t id = ((uint32_t)t.rx_data[0] << 16) | ((uint32_t)t.rx_data[1] << 8) | t.rx_data[2];
    return id;
}

// 3) Wait until Ready (WIP=0)
esp_err_t flash_wait_ready(uint32_t timeout_ms)
{
    uint32_t start = xTaskGetTickCount() * portTICK_PERIOD_MS;
    while (1)
    {
        uint8_t sr = flash_read_status();
        if ((sr & 0x01) == 0) return ESP_OK; // Bit 0 is WIP

        if ((xTaskGetTickCount() * portTICK_PERIOD_MS - start) > timeout_ms)
            return ESP_ERR_TIMEOUT;
            
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

// 4) Sector Erase
esp_err_t flash_erase_sector(uint32_t addr)
{
    flash_write_enable();
    // Wait for WEL bit to set (critical check)
    if (!(flash_read_status() & 0x02)) {
        printf("Error: Write Enable Latch (WEL) failed to set before Erase!\n");
        return ESP_FAIL;
    }

    spi_transaction_ext_t t = {0};
    t.base.cmd = 0x20;
    t.base.addr = addr;
    t.base.flags = SPI_TRANS_VARIABLE_ADDR;
    t.address_bits = 24;
    
    esp_err_t err = spi_device_transmit(flash, &t.base);
    if (err != ESP_OK) return err;

    return flash_wait_ready(1000);
}

// 5) Page Program
esp_err_t flash_page_program(uint32_t addr, const uint8_t *data, size_t len)
{
    flash_write_enable();
    // Wait for WEL bit to set (critical check)
    if (!(flash_read_status() & 0x02)) {
        printf("Error: Write Enable Latch (WEL) failed to set before Program!\n");
        return ESP_FAIL;
    }

    spi_transaction_ext_t t = {0}; 
    t.base.cmd = 0x02;
    t.base.addr = addr;
    t.base.flags = SPI_TRANS_VARIABLE_ADDR;
    t.address_bits = 24;
    t.base.tx_buffer = data;
    t.base.length = len * 8;

    esp_err_t err = spi_device_transmit(flash, &t.base);
    if (err != ESP_OK) return err;

    return flash_wait_ready(100);
}

// 6) Read Data
esp_err_t flash_read_data(uint32_t addr, uint8_t *buf, size_t len)
{
    spi_transaction_ext_t t = {0};
    t.base.cmd = 0x03;
    t.base.addr = addr;
    t.base.flags = SPI_TRANS_VARIABLE_ADDR;
    t.address_bits = 24;
    t.base.length = len * 8;
    t.base.rxlength = len * 8;
    t.base.rx_buffer = buf;

    return spi_device_transmit(flash, &t.base);
}

// 7) Unprotect
esp_err_t flash_global_unprotect()
{
    flash_write_enable();
    uint8_t status = 0x00;
    spi_transaction_t t = {0};
    t.cmd = 0x01;
    t.tx_buffer = &status;
    t.length = 8;
    
    return spi_device_transmit(flash, &t);
}

void app_main(void)
{
    // GPIO Init
    gpio_set_direction(WP_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(WP_PIN, 1);
    gpio_set_direction(HOLD_PIN, GPIO_MODE_OUTPUT);
    gpio_set_level(HOLD_PIN, 1);

    spi_bus_config_t buscfg = {
        .mosi_io_num = MOSI_PIN,
        .miso_io_num = MISO_PIN,
        .sclk_io_num = SCLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };

    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 4 * 1000 * 1000, // 4 MHz safe speed
        .mode = 0,
        .spics_io_num = CS_PIN,
        .queue_size = 1,
        .command_bits = 8,
        .address_bits = 0,
        .cs_ena_posttrans = 3, // CRITICAL FIX: Keep CS high for 3 cycles after transaction
    };

    ESP_ERROR_CHECK(spi_bus_initialize(SPI3_HOST, &buscfg, SPI_DMA_CH_AUTO));
    ESP_ERROR_CHECK(spi_bus_add_device(SPI3_HOST, &devcfg, &flash));

    printf("--- SPI Flash Test ---\n");

    // 1. Check ID
    uint32_t id = flash_read_id();
    printf("JEDEC ID: 0x%06X\n", (unsigned int)id);
    if (id == 0x000000 || id == 0xFFFFFF) {
        printf("ERROR: Flash not detected! Check Wiring (MISO/MOSI/CS/CLK).\n");
        return;
    }

    // 2. Unprotect
    flash_global_unprotect();
    flash_wait_ready(100);

    // 3. Prepare Data
    uint8_t *page = heap_caps_malloc(256, MALLOC_CAP_DMA);
    uint8_t *readbuf = heap_caps_calloc(256, 1, MALLOC_CAP_DMA);
    for (int i = 0; i < 256; i++) page[i] = i;

    // 4. Erase
    printf("Erasing...\n");
    if (flash_erase_sector(0x000000) != ESP_OK) return;

    // 5. Write
    printf("Writing...\n");
    if (flash_page_program(0x000000, page, 256) != ESP_OK) return;

    // 6. Read
    printf("Reading...\n");
    flash_read_data(0x000000, readbuf, 256);

    printf("First 8 bytes: ");
    for (int i = 0; i < 8; i++) printf("%02X ", readbuf[i]);
    printf("\n");

    if (memcmp(page, readbuf, 256) == 0) {
        printf("SUCCESS: Data Verified!\n");
    } else {
        printf("FAILURE: Data Mismatch.\n");
    }

    free(page);
    free(readbuf);
}