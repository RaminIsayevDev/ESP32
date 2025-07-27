#include "functions.h"
#include "globals.h"
#include "esp_log.h"
#include "st7735s.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Functions below:

void st7735s_send_gamma_profile( const uint8_t *gamma_pos, size_t gamma_pos_len,
                                 const uint8_t *gamma_neg, size_t gamma_neg_len)
{
    st7735s_send_cmd(ST7735_GMCTRP1);
    st7735s_send_data(gamma_pos, gamma_pos_len);

    st7735s_send_cmd(ST7735_GMCTRN1);
    st7735s_send_data(gamma_neg, gamma_neg_len);
}

// Tasks below:

void LCD_Display_Task(void* pvParameters) {
    // Send positive and negative gammas'
    st7735s_send_gamma_profile(GAMMA_P_ARRAY, GAMMA_P_ARRAY_LEN, GAMMA_N_ARRAY, GAMMA_N_ARRAY_LEN);

    lv_color_t blue_color = lv_color_make(0, 0, 255);  // Pure blue

    lv_obj_t *screen = lv_scr_act();  // Get current screen

    lv_obj_set_style_bg_color(screen, lv_color_make(0, 0, 255), 0);
    lv_obj_set_style_bg_opa(screen, LV_OPA_COVER, 0);


    while(1) {
        lv_timer_handler();  // Process LVGL tasks
        vTaskDelay(pdMS_TO_TICKS(10));  // Call periodically
    }
}