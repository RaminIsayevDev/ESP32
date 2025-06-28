#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <esp_log.h>
#include <nvs_flash.h>
#include <driver/gpio.h>
#include <stdio.h>

#include "myWifi.h" // Include our custom Wi-Fi module header

const gpio_num_t LED_PIN_1 = GPIO_NUM_4;

void Task1_blink_Led (void *pvParameters) {
    (void) pvParameters;

    gpio_set_direction(LED_PIN_1, GPIO_MODE_OUTPUT);

    for(;;) {
        gpio_set_level(LED_PIN_1, 1);
        printf("Led is turned on\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        
        gpio_set_level(LED_PIN_1, 0);
        printf("Led is turned off\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

// Ensure app_main has C linkage if compiling as C++
void app_main(void)
{
    // Initialize NVS (Non-Volatile Storage)
    // NVS is used to store Wi-Fi credentials and other persistent data
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase()); // Erase NVS if corrupted or new version
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret); // Check for other NVS initialization errors

    // Initialize our custom Wi-Fi module
    wifi_station_init();

    // Create the LED blinking task
    xTaskCreate(
        Task1_blink_Led,
        "Blink_LED_PIN_4",
        2048,
        NULL,
        10,
        NULL
    );
}
