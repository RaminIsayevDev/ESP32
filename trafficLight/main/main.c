#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define led_RED GPIO_NUM_2
#define led_YELLOW GPIO_NUM_4
#define led_GREEN GPIO_NUM_5

#define delay_Time_between_blinks 1500
#define delay_Time_RED 5000
#define delay_Time_YELLOW 2000
#define delay_Time_GREEN 4000
#define delay_Time_blinking_GREEN_led 1000

void init_led(int pin) {
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
}

void delay(int time){
    vTaskDelay(pdMS_TO_TICKS(time));
}

void Task1_blink_traffic_lights(void *pvParameters) {
    (void) pvParameters;

    for(;;) {
        gpio_set_level(led_RED, 1);         // Turn ON the RED led
        delay(delay_Time_RED);

        delay(delay_Time_between_blinks);   // Turn OFF the RED led
        gpio_set_level(led_RED, 0);

        gpio_set_level(led_YELLOW, 1);      // Turn ON the YELLOW led
        delay(delay_Time_YELLOW);

        delay(delay_Time_between_blinks);   // Turn OFF the YELLOW led
        gpio_set_level(led_YELLOW, 0);

        gpio_set_level(led_GREEN, 1);      // Turn ON the GREEN led
        delay(delay_Time_GREEN);

        delay(delay_Time_between_blinks);   // Turn OFF the GREEN led and start to blinking
        gpio_set_level(led_GREEN, 0);

        for(int i = 0; i < 5; i++) {
            gpio_set_level(led_GREEN, 1);
            delay(delay_Time_blinking_GREEN_led);
            gpio_set_level(led_GREEN, 0);
            delay(delay_Time_blinking_GREEN_led);
        }

    }
}

void app_main(void) {
    init_led(led_RED);
    init_led(led_YELLOW);
    init_led(led_GREEN);

    xTaskCreate(
        Task1_blink_traffic_lights,
        "Task1_blink_traffic_lights",
        2048,
        NULL,
        5,
        NULL
    );
}