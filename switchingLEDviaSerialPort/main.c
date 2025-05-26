#include <stdbool.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define GPIO_LED_PIN 2
#define DELAY_TIME 1000

void switch_led(bool state) {                  // TRUE turns ON the LED, FALSE turns OFF
    if (state == true) {
        gpio_set_level(GPIO_LED_PIN, 1);
    } else if (state == false) {
        gpio_set_level(GPIO_LED_PIN, 0);
    }
}

void init_led(int pin) {                    // function for initializing the LED
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
}

bool read_the_Input(bool *newState) {                 
    char inputString[10];
    scanf("%9s", inputString);

    if ((strcmp(inputString, "ON") == 0) || 
        (strcmp(inputString, "on") == 0) || 
        (strcmp(inputString, "On") == 0)) {
        *newState = true;
        return true;
    } else if ((strcmp(inputString, "OFF") == 0) || 
               (strcmp(inputString, "off") == 0) || 
               (strcmp(inputString, "Off") == 0)) {
        *newState = false;
        return true;
    } else {
        printf("Invalid input! Please type ON or OFF.\n");
        return false;
    }
}

void app_main(void) {
    init_led(GPIO_LED_PIN);
    bool ledState = false;

    printf("Turning LED on - ON \n Turning LED off - OFF \n");

    while(1) {
        bool validInput = read_the_Input(&ledState);

        if (validInput) {
            if (ledState) {
                printf("LED is turned ON\n");
            } else {
                printf("LED is turned OFF\n");
            }
            switch_led(ledState);
        }
        vTaskDelay(DELAY_TIME / portTICK_PERIOD_MS);
    }
}