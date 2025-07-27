#include <freertos/FreeRTOS.h>
#include "freertos/task.h"

// #include "driver/gpio.h"
#include "esp_err.h"      
#include "lvgl.h"
#include "st7735s.h"

// My libraries
#include "globals.h"
#include "functions.h"

#define DISP_BUF_LINES 40  // Кол-во строк буфера (или подбери своё)
#define LVGL_BUF_SIZE (LV_HOR_RES_MAX * DISP_BUF_LINES)

void app_main(void) {

    static lv_disp_draw_buf_t draw_buf;       // ✅ Структура буфера LVGL
    static lv_color_t buf1[LVGL_BUF_SIZE];   
    
    
    lv_init();

    lvgl_driver_init();

    st7735s_init();

    lv_disp_draw_buf_init(&draw_buf, &buf1, NULL, LV_HOR_RES_MAX * DISP_BUF_LINES);

    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = st7735s_flush;
    disp_drv.hor_res = 128;
    disp_drv.ver_res = 160;
    disp_drv.draw_buf = &draw_buf;
    lv_disp_drv_register(&disp_drv);

    xTaskCreate(LCD_Display_Task, "Display_LCD", 4096, NULL, 5, NULL);
}