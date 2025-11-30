#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "tm1637.h"
#include <time.h>

#include "display.h"
#include "features/rtc/rtc.h"

void display_task(void* pvParameters) {
  while(1) {
    
    int hour = RTC_timeinfo.tm_hour;
    int minute = RTC_timeinfo.tm_min;

    int display_value = hour * 100 + minute;

    tm1637_set_number(led, display_value, true, 0x04); // colon ON
    vTaskDelay(pdMS_TO_TICKS(500));

    tm1637_set_number(led, display_value, true, 0);    // colon OFF
    vTaskDelay(pdMS_TO_TICKS(500));
  }
}