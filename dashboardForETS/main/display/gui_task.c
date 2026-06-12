#include <lvgl.h>
#include "drivers/display/st7735/lv_st7735.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "include/tachometer.h"
#include "spi/include/spi_pins.h"

// Buffer for drawing: keep under 4092 byte SPI transaction limit
// 128 * 15 * 2 = 3840 bytes per buffer (15 rows in RGB565)
#define BUF_SIZE (128 * 15)

// SPI device handle for the display
static spi_device_handle_t spi_dev;

// Set DC pin for command (0) or data (1)
static inline void st7735_set_dc(bool is_data)
{
    if (is_data) {
        gpio_set_level(PIN_NUM_DC, 1);
    } else {
        gpio_set_level(PIN_NUM_DC, 0);
    }
}

// Platform-dependent callback to send a command to the LCD controller (polling)
static void st7735_send_cmd(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size,
                            const uint8_t *param, size_t param_size)
{
    st7735_set_dc(false);  // DC = 0 for command
    spi_transaction_t t = {
        .length = cmd_size * 8,
        .tx_buffer = cmd,
    };
    spi_device_transmit(spi_dev, &t);

    if (param && param_size > 0) {
        st7735_set_dc(true);  // DC = 1 for data
        spi_transaction_t td = {
            .length = param_size * 8,
            .tx_buffer = param,
        };
        spi_device_transmit(spi_dev, &td);
    }
}

// Platform-dependent callback to send pixel data to the LCD controller
static void st7735_send_color(lv_display_t *disp, const uint8_t *cmd, size_t cmd_size,
                              uint8_t *param, size_t param_size)
{
    // First send the RAM_WR command (cmd = 0x2C)
    if (cmd && cmd_size > 0) {
        st7735_set_dc(false);  // DC = 0 for command
        spi_transaction_t tc = {
            .length = cmd_size * 8,
            .tx_buffer = cmd,
        };
        spi_device_transmit(spi_dev, &tc);
    }

    // Then send the pixel data (DC = 1)
    if (param && param_size > 0) {
        st7735_set_dc(true);  // DC = 1 for data
        spi_transaction_t td = {
            .length = param_size * 8,
            .tx_buffer = param,
        };
        spi_device_transmit(spi_dev, &td);
    }

    // Notify LVGL that flush is complete
    lv_display_flush_ready(disp);
}

// Tick increment counter for LVGL
static volatile uint32_t lv_tick_ms = 0;

static void lv_tick_timer_cb(void *arg)
{
    lv_tick_ms += 1;
}

// Wrapper to get milliseconds for LVGL
static uint32_t lv_tick_esp_get_ms(void)
{
    return lv_tick_ms;
}

void gui_task(void *pvParameters)
{
    // Initialize the SPI device for the ST7735
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 26 * 1000 * 1000,  // 26 MHz (ESP32 max SPI clock)
        .mode = 0,                            // SPI mode 0
        .spics_io_num = PIN_NUM_CS,
        .queue_size = 1,
        .flags = 0,
        .pre_cb = NULL,
        .post_cb = NULL,
    };

    esp_err_t ret = spi_bus_add_device(LCD_HOST, &devcfg, &spi_dev);
    assert(ret == ESP_OK);

    lv_init();

    // Create a periodic timer to increment the LVGL tick (1 ms period)
    const esp_timer_create_args_t tick_timer_args = {
        .callback = &lv_tick_timer_cb,
        .name = "lvgl_tick"
    };
    esp_timer_handle_t tick_timer;
    ESP_ERROR_CHECK(esp_timer_create(&tick_timer_args, &tick_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(tick_timer, 1000)); // 1 ms

    // Register tick callback
    lv_tick_set_cb(lv_tick_esp_get_ms);

    // Create the ST7735 display using LVGL's built-in driver
    uint32_t hor_res = LCD_H_RES;  // 128
    uint32_t ver_res = LCD_V_RES;  // 160

    // Setup DC pin as GPIO output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << PIN_NUM_DC),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    gpio_config(&io_conf);

    // Reset the display if RST pin is configured
    if (PIN_NUM_RST >= 0) {
        gpio_config_t rst_conf = {
            .pin_bit_mask = (1ULL << PIN_NUM_RST),
            .mode = GPIO_MODE_OUTPUT,
            .pull_up_en = GPIO_PULLUP_DISABLE,
            .pull_down_en = GPIO_PULLDOWN_DISABLE,
            .intr_type = GPIO_INTR_DISABLE,
        };
        gpio_config(&rst_conf);

        gpio_set_level(PIN_NUM_RST, 0);
        vTaskDelay(pdMS_TO_TICKS(10));
        gpio_set_level(PIN_NUM_RST, 1);
        vTaskDelay(pdMS_TO_TICKS(120));
    }

    // Create the display with LVGL ST7735 driver
    lv_display_t *disp = lv_st7735_create(hor_res, ver_res, LV_LCD_FLAG_BGR,
                                          st7735_send_cmd, st7735_send_color);
    assert(disp != NULL);

    // Set up the draw buffer
    lv_color_t *buf1 = malloc(BUF_SIZE * sizeof(lv_color_t));
    lv_color_t *buf2 = malloc(BUF_SIZE * sizeof(lv_color_t));
    assert(buf1 != NULL && buf2 != NULL);

    lv_display_set_buffers(disp, buf1, buf2, BUF_SIZE * sizeof(lv_color_t), LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Create the tachometer UI
    create_tachometer();

    // LVGL loop — feed watchdog and handle LVGL timers
    while (1) {
        lv_timer_handler();
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}