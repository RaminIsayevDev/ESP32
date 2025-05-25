//  #include <stdio.h>                      // Standard input/output functions (not strictly needed here)
#include "freertos/FreeRTOS.h"          // FreeRTOS main header
#include "freertos/task.h"              // FreeRTOS task functions (for vTaskDelay)
#include "driver/gpio.h"                // ESP32 GPIO functions
//  #include "esp_log.h"                    // ESP32 logging (not used in this example, but often included)

#define BLINK_GPIO 2           // The GPIO pin number connected to your LED (change if needed)
#define BLINK_PERIOD_MS 300   // Blink interval in milliseconds (1000ms = 1 second)

void app_main(void)
{
    // Reset the GPIO pin (sets it to a known state)
    gpio_reset_pin(BLINK_GPIO);

    // Set the GPIO pin as an output
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    int led_state = 0; // Variable to track LED state: 0 = off, 1 = on

    while (1) {
        // Set the GPIO pin to the current LED state (on or off)
        gpio_set_level(BLINK_GPIO, led_state);

        // Toggle the LED state for next loop
        led_state = !led_state;

        // Wait for the specified period (in milliseconds)
        vTaskDelay(BLINK_PERIOD_MS / portTICK_PERIOD_MS);
    }
}