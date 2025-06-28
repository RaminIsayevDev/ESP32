// Required FreeRTOS headers for task management
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// ESP-IDF headers for GPIO control and general system functions
#include <driver/gpio.h> // For GPIO functions like gpio_set_direction, gpio_set_level
#include <stdio.h>      // For printf (standard C library for console output)
#include <esp_system.h> // For general ESP-IDF system functions (though not strictly needed for this basic example)

// Define the GPIO pins for the LEDs
const gpio_num_t LED_PIN_1 = GPIO_NUM_2; // Typically the built-in LED on many ESP32 boards
const gpio_num_t LED_PIN_2 = GPIO_NUM_4; // Another common GPIO pin, you can change this

// Task handles are optional but useful if you need to refer to or control tasks later
// e.g., to delete, suspend, or resume a task from another part of your code.
TaskHandle_t Task1_Handle = NULL;
TaskHandle_t Task2_Handle = NULL;

/*
 * Task 1: Blinks LED_PIN_1 at a slower rate.
 *
 * All FreeRTOS tasks must have this function signature:
 * void TaskName(void *pvParameters)
 *
 * - pvParameters: A pointer to any data you want to pass to the task during its creation.
 * In this simple example, we are not passing any specific data, so it's NULL.
 */
void Task1_Blink(void *pvParameters) {
  // Cast pvParameters to the expected type if you were passing data.
  // In this case, we don't need it as pvParameters is NULL.
  (void) pvParameters; // Suppress unused parameter warning

  // Set the LED pin as an output using ESP-IDF GPIO functions
  gpio_set_direction(LED_PIN_1, GPIO_MODE_OUTPUT);

  // Tasks typically run in an infinite loop.
  // This is crucial for RTOS tasks, as they don't "return" like regular functions.
  for (;;) {
    gpio_set_level(LED_PIN_1, 1);   // Turn LED on (1 for HIGH)
    printf("Task 1: LED 1 ON\n");
    // vTaskDelay() is the FreeRTOS equivalent of delay().
    // It yields control to the FreeRTOS scheduler, allowing other tasks to run.
    // pdMS_TO_TICKS() macro converts milliseconds to RTOS ticks, which is safer.
    vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for 1000 milliseconds (1 second)

    gpio_set_level(LED_PIN_1, 0);    // Turn LED off (0 for LOW)
    printf("Task 1: LED 1 OFF\n");
    vTaskDelay(pdMS_TO_TICKS(1000)); // Wait for 1000 milliseconds (1 second)
  }
}

/*
 * Task 2: Blinks LED_PIN_2 at a faster rate.
 * Similar structure to Task 1, but with a different delay.
 */
void Task2_Blink(void *pvParameters) {
  (void) pvParameters; // Suppress unused parameter warning

  gpio_set_direction(LED_PIN_2, GPIO_MODE_OUTPUT);

  for (;;) {
    gpio_set_level(LED_PIN_2, 1);   // Turn LED on
    printf("Task 2: LED 2 ON\n");
    vTaskDelay(pdMS_TO_TICKS(250)); // Wait for 250 milliseconds

    gpio_set_level(LED_PIN_2, 0);    // Turn LED off
    printf("Task 2: LED 2 OFF\n");
    vTaskDelay(pdMS_TO_TICKS(250)); // Wait for 250 milliseconds
  }
}

// The main entry point for ESP-IDF applications.
// This function runs on the main task that ESP-IDF sets up.
void app_main(void) {
  // Initialize serial communication implicitly (ESP-IDF handles this by default).
  // No explicit Serial.begin() is needed like in Arduino.
  printf("FreeRTOS ESP32 Blink Example Starting (ESP-IDF)...\n");

  /*
   * xTaskCreatePinnedToCore() is used to create a new task and pin it to a specific CPU core.
   * The ESP32 is dual-core (Core 0 and Core 1).
   *
   * Parameters:
   * 1. Task Function (TaskFunction_t): The name of the function that implements the task.
   * 2. Task Name (const char * const): A descriptive name for the task (useful for debugging).
   * 3. Stack Depth (configSTACK_DEPTH_TYPE): The size of the stack for the task in bytes.
   * 4. Parameters (void *): A pointer to any data to be passed to the task. Use NULL if none.
   * 5. Priority (UBaseType_t): The priority of the task. Higher numbers mean higher priority.
   * 6. Task Handle (TaskHandle_t *): A pointer to a variable that will store the task's handle. Use NULL if not needed.
   * 7. Core ID (BaseType_t): The core to pin the task to (0 for PRO_CPU, 1 for APP_CPU).
   * Use tskNO_AFFINITY to let the scheduler decide.
   */

  // Create Task 1 and pin it to Core 0 (PRO_CPU)
  xTaskCreatePinnedToCore(
    Task1_Blink,    // Task function
    "LED_Blink_1",  // Task name
    2048,           // Stack size (bytes) - adjust if needed for more complex tasks
    NULL,           // Parameter to pass
    1,              // Task priority (lower priority)
    &Task1_Handle,  // Task handle
    0               // Core 0
  );

  // Create Task 2 and pin it to Core 1 (APP_CPU)
  xTaskCreatePinnedToCore(
    Task2_Blink,    // Task function
    "LED_Blink_2",  // Task name
    2048,           // Stack size (bytes) - adjust if needed for more complex tasks
    NULL,           // Parameter to pass
    2,              // Task priority (higher priority)
    &Task2_Handle,  // Task handle
    1               // Core 1
  );

  // The app_main function can either return (and the main task will self-delete)
  // or it can enter an infinite loop itself if it has work to do.
  // For this example, we let the tasks handle the main logic, so app_main
  // could theoretically exit, but for embedded systems, it's common for
  // app_main to block or loop if there are no other tasks.
  // In this case, since the created tasks run indefinitely, app_main will
  // effectively return after creating them, and FreeRTOS will continue to schedule.
}
