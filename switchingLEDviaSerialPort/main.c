#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define GPIO_LED_PIN 2

void app_main(void) {
    gpio_reset_pin(GPIO_LED_PIN);
    gpio_set_direction(GPIO_MODE_OUTPUT);

    
}