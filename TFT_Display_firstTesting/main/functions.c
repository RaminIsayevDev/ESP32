#include "functions.h"
#include "globals.h"
#include "esp_lcd_io_spi.h"
#include "esp_lcd_panel_dev.h"
#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "driver/spi_master.h"

// Functions below:

void send_gamma_config(esp_lcd_panel_io_handle_t io_handle)
{
    esp_err_t ret;

    // Отправляем команду положительной гамма-коррекции
    ret = esp_lcd_panel_io_tx_param(io_handle, ST7735S_GMCTRP1_CMD, GAMMA_P_ARRAY, GAMMA_P_ARRAY_LEN);
    if (ret != ESP_OK) {
        ESP_LOGE("ST7735S", "Не удалось отправить команду положительной гаммы: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI("ST7735S", "Команда положительной гаммы успешно отправлена.");
    }

    // Отправляем команду отрицательной гамма-коррекции
    ret = esp_lcd_panel_io_tx_param(io_handle, ST7735S_GMCTRN1_CMD, GAMMA_N_ARRAY, GAMMA_N_ARRAY_LEN);
    if (ret != ESP_OK) {
        ESP_LOGE("ST7735S", "Не удалось отправить команду отрицательной гаммы: %s", esp_err_to_name(ret));
    } else {
        ESP_LOGI("ST7735S", "Команда отрицательной гаммы успешно отправлена.");
    }
}

void fill_screen_blue_by_strips(esp_lcd_panel_handle_t panel_handle) {
    uint16_t *line_buffer = (uint16_t *)heap_caps_malloc(LCD_H_RES * LINE_BUFFER_HEIGHT * sizeof(uint16_t), MALLOC_CAP_DMA);
    if (line_buffer == NULL) {
        ESP_LOGE("ST7735S", "Не удалось выделить буфер для полосы!");
        return;
    }

    // Заполняем буфер одной полосы синим цветом
    for (int i = 0; i < LCD_H_RES * LINE_BUFFER_HEIGHT; i++) {
        line_buffer[i] = 0x001F;
    }

    for (int y = 0; y < LCD_V_RES; y += LINE_BUFFER_HEIGHT) {
        int h = (y + LINE_BUFFER_HEIGHT > LCD_V_RES) ? (LCD_V_RES - y) : LINE_BUFFER_HEIGHT;
        esp_lcd_panel_draw_bitmap(
            panel_handle,
            0, y,
            LCD_H_RES, y + h,
            line_buffer
        );
    }
}

// Tasks below:

void LCD_Display_Task(void* pvParameters) {
    // Set up display before using
    esp_lcd_panel_reset(panel_handle);
    esp_lcd_panel_init(panel_handle);

    // Send positive and negative gammas'
    send_gamma_config(io_handle);

    // turn on the display
    esp_lcd_panel_disp_on_off(panel_handle, true);

    while(1) {
        fill_screen_blue_by_strips(panel_handle);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

