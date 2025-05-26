#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include <stdbool.h>

#define GPIO_LED_PIN 2
#define DELAY_TIME 10

void switch_led(bool state) {                  // TRUE turns ON the LED, FALSE turns OFF
    if (state == true) {
        gpio_set_level(GPIO_LED_PIN, 1);
    } else if (state == false) {
        gpio_set_level(GPIO_LED_PIN, 0);
    }
}

void init_led(int pin) {                    // function for initializing the LED
    gpio_reset_pin(pin);
    gpio_set_direction(GPIO_MODE_OUTPUT);
}

bool read_the_Input(void) {                 // function returns TRUE if input ON, and FALSE if OFF
    char inputString[];
    scanf("%s", inputString);

    if ((inputString == "ON") || (inputString == "on") || (inputString == "On")) {
        return true;
    } else if ((inputString == "OFF") || (inputString == "off") || (inputString == "Off")) {
        return false;
    }
}

void app_main(void) {
    init_led(GPIO_LED_PIN);
    bool inputAction = false;

    printf("Turning LED on - ON \n Turning LED off - OFF \n");

    while(1) {
        inputAction = read_the_Input();

        if (inputAction == true) {
            printf("LED is turned ON\n");
        } else if (inputAction == false) {
            printf("LED is turned OFF");
        }

        switch_led(inputAction);

        vTaskDelay(DELAY_TIME / portTICK_PERIOD_MS);
    }
}