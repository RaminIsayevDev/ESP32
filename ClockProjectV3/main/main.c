#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include <time.h>
#include "driver/gpio.h"

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

void st7735s_send_gamma_profile( const uint8_t *gamma_pos, size_t gamma_pos_len,
                                 const uint8_t *gamma_neg, size_t gamma_neg_len)
{
    st7735s_send_cmd(ST7735_GMCTRP1);
    st7735s_send_data(gamma_pos, gamma_pos_len);

    st7735s_send_cmd(ST7735_GMCTRN1);
    st7735s_send_data(gamma_neg, gamma_neg_len);
}

void app_main(void) {
    // Слушатель всех event-ов
    ESP_ERROR_CHECK(esp_event_loop_create_default());
  
    // Register event handler after event loop is created
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_CONNECTED, &my_wifi_connected_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_START, &my_wifi_started_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &my_wifi_disconnected_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &my_wifi_ip_handler, NULL));

    static lv_disp_draw_buf_t draw_buf;      
    static lv_color_t buf1[LVGL_BUF_SIZE]; 

    DisplayMessage msg;

    SNTP_to_RTC_Queue = xQueueCreate(5, sizeof(struct tm));

    lv_init();

    lvgl_driver_init();

    st7735s_init();

    lv_disp_draw_buf_init(&draw_buf, &buf1, NULL, LV_HOR_RES_MAX * DISP_BUF_LINES);

    lv_disp_drv_init(&disp_drv);

    disp_drv.rotated = LV_DISP_ROT_90;

    disp_drv.flush_cb = st7735s_flush;
    disp_drv.hor_res = 160;
    disp_drv.ver_res = 128;
    disp_drv.draw_buf = &draw_buf;

    lv_disp_drv_register(&disp_drv);

    st7735s_send_gamma_profile(GAMMA_P_ARRAY, GAMMA_P_ARRAY_LEN, GAMMA_N_ARRAY, GAMMA_N_ARRAY_LEN);

    toDisplay_Queue = xQueueCreate(5, sizeof(DisplayMessage));

    // А вот теперь можно запускать задачу:
    xTaskCreate(display_task, "display_task", 4096, NULL, 10, NULL);

    // Create wifi_task
    xTaskCreate(wifi_task, "Wifi task", 4096, NULL, 10, NULL);

}