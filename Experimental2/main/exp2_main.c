#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define GPIO_led 4
#define GPIO_button 2
#define delay_time 10

void app_main(void) {
    gpio_reset_pin(GPIO_led);                           //  set's pins to the known state
    gpio_reset_pin(GPIO_button);

    gpio_set_direction(GPIO_led, GPIO_MODE_OUTPUT);
    gpio_set_direction(GPIO_button, GPIO_MODE_INPUT);   //  set's direction of pins

    while(1) {
        int state_of_input = gpio_get_level(GPIO_button);

        if (state_of_input) {
            gpio_set_level(GPIO_led, 1);
            vTaskDelay(delay_time / portTICK_PERIOD_MS);
        } else {
            gpio_set_level(GPIO_led, 0);
            vTaskDelay(delay_time / portTICK_PERIOD_MS);
        }
    }
}