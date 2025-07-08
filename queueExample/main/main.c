#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <freertos/queue.h>

// Объявление глобального хендла для очереди
static QueueHandle_t xQueue = NULL;

// ----- ЗАДАЧА 1: ОТПРАВИТЕЛЬ -----
// Эта задача отправляет данные в очередь
void vSenderTask(void *pvParameters) {
    int32_t lValueToSend = 0;
    BaseType_t xStatus;

    for (;;) {
        xStatus = xQueueSend(xQueue, &lValueToSend, 0);

        if (xStatus != pdPASS) {
            printf("Could not send to the queue.\n");
        } else {
            printf("Sent value: %ld\n", lValueToSend);
            lValueToSend++;
        }
        
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ----- ЗАДАЧА 2: ПОЛУЧАТЕЛЬ -----
// Эта задача получает данные из очереди
void vReceiverTask(void *pvParameters) {
    int32_t lReceivedValue;
    BaseType_t xStatus;
    const TickType_t xTicksToWait = pdMS_TO_TICKS(100);

    for (;;) {
        xStatus = xQueueReceive(xQueue, &lReceivedValue, xTicksToWait);

        if (xStatus == pdPASS) {
            printf("Received value: %ld\n", lReceivedValue);
        } else {
            printf("Could not receive from the queue.\n");
        }
    }
}

void app_main(void) {
    // В ESP-IDF базовая инициализация (как UART для printf)
    // выполняется до вызова app_main, поэтому prvSetupHardware() не требуется.

    // Создаем очередь, которая может хранить 5 элементов типа int32_t.
    xQueue = xQueueCreate(5, sizeof(int32_t));

    if (xQueue != NULL) {
        // Создаем две задачи.
        // Используем более безопасный размер стека для ESP32.
        xTaskCreate(vSenderTask,
                    "Sender",
                    2048,          // Размер стека в словах (2048 * 4 = 8192 байт)
                    NULL,
                    1,
                    NULL);

        xTaskCreate(vReceiverTask,
                    "Receiver",
                    2048,          // Размер стека в словах
                    NULL,
                    2,             // У получателя приоритет выше
                    NULL);

    } else {
        printf("Queue could not be created.\n");
    }
}