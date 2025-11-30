#include <freertos/FreeRTOS.h>
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "esp-idf-ds3231.h"

#include "rtc.h"
#include "core/queue/queue.h"
#include "features/display/display.h"

#define SDA_PIN GPIO_NUM_21
#define SCL_PIN GPIO_NUM_22

i2c_master_bus_handle_t i2c_bus = NULL;

struct tm RTC_timeinfo = {0};

void RTC_task(void *pvParameters) { // Task for set, save and output time when needed

  i2c_master_bus_config_t i2c_bus_config = {
    .clk_source = I2C_CLK_SRC_DEFAULT,
    .i2c_port = I2C_NUM_0,
    .scl_io_num = SCL_PIN,
    .sda_io_num = SDA_PIN,
    .glitch_ignore_cnt = 7,
    .flags.enable_internal_pullup = true,
  };

  ESP_ERROR_CHECK(i2c_new_master_bus(&i2c_bus_config, &i2c_bus));

  rtc_handle_t *rtc = ds3231_init(&i2c_bus);

  
  float temp = 0;

  if (xQueueReceive(queue_SNTP_to_RTC_data, &RTC_timeinfo, portMAX_DELAY) == pdTRUE) {
    //printf("Received: %d\n", received_value);
    ESP_ERROR_CHECK(ds3231_time_tm_set(rtc, RTC_timeinfo));
    xTaskCreate(display_task, "display_time", 1024, NULL, 10, NULL);
  }

  while(1) {
    struct tm* current_time = ds3231_time_get(rtc);
    RTC_timeinfo = *current_time;
    free(current_time);
    temp = ds3231_temperature_get(rtc);
    
    printf("%04d-%02d-%02d %02d:%02d:%02d Temp: %.2f\n", RTC_timeinfo.tm_year + 1900 /*Add 1900 for better readability*/, RTC_timeinfo.tm_mon + 1,
            RTC_timeinfo.tm_mday, RTC_timeinfo.tm_hour, RTC_timeinfo.tm_min, RTC_timeinfo.tm_sec, temp);
    vTaskDelay(pdMS_TO_TICKS(60000));
  }
}