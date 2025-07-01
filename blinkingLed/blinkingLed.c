/*
 * Simple LED Blink Example for ESP-IDF
 *
 * This example demonstrates the basic steps to configure a GPIO pin as an
 * output and toggle it to make an LED blink.
 *
 * Target: Any ESP32 board
 * ESP-IDF Version: v4.x or newer
 *
 * To use this code:
 * 1. Save it as `main.c` inside the `main` directory of your ESP-IDF project.
 * 2. Change the `BLINK_GPIO` definition if your LED is connected to a
 * different GPIO pin. Most development boards have a built-in LED
 * on GPIO 2.
 * 3. Build and flash the project using the ESP-IDF command-line tools:
 * idf.py build
 * idf.py -p (your port) flash monitor
 */

#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"

// Define the GPIO pin where the LED is connected.
// On many ESP32 development boards, this is GPIO 2.
#define BLINK_GPIO CONFIG_BLINK_GPIO

// Define a tag for logging, which helps in debugging.
static const char *TAG = "BLINK";

// The main function for the application, which is the entry point.
void app_main(void)
{
    ESP_LOGI(TAG, "Configuring GPIO pin for blinking...");

    // --- Configure the GPIO pin ---

    // Reset the pin to a known state (optional but good practice).
    gpio_reset_pin(BLINK_GPIO);
    
    // Set the GPIO direction to output.
    // This allows us to send signals (voltage) out from the pin.
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    ESP_LOGI(TAG, "GPIO configuration complete. Starting blink loop.");

    // --- Infinite Loop for Blinking ---
    while (1) {
        // Turn the LED ON.
        // `gpio_set_level` sends a high signal (3.3V) to the GPIO pin.
        gpio_set_level(BLINK_GPIO, 1);
        ESP_LOGI(TAG, "LED is ON");

        // Wait for 1000 milliseconds (1 second).
        // `vTaskDelay` is a FreeRTOS function that pauses the current task
        // without blocking the entire CPU. The argument is in "ticks".
        // `pdMS_TO_TICKS` is a handy macro to convert milliseconds to ticks.
        vTaskDelay(pdMS_TO_TICKS(1000));

        // Turn the LED OFF.
        // `gpio_set_level` sends a low signal (0V) to the GPIO pin.
        gpio_set_level(BLINK_GPIO, 0);
        ESP_LOGI(TAG, "LED is OFF");

        // Wait for another 1000 milliseconds.
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
