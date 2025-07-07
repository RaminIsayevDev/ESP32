#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"

// Объявление глобального хендла для очереди
static QueueHandle_t xQueue = NULL;

// ----- ЗАДАЧА 1: ОТПРАВИТЕЛЬ -----
// Эта задача отправляет данные в очередь
void vSenderTask(void *pvParameters) {
    int32_t lValueToSend = 0;
    BaseType_t xStatus;

    for (;;) {
        // Отправляем значение в очередь.
        // Последний параметр (ticks to wait) установлен в 0.
        // Если очередь полна, задача не будет ждать.
        xStatus = xQueueSend(xQueue, &lValueToSend, 0);

        if (xStatus != pdPASS) {
            // Не удалось отправить данные, возможно, очередь заполнена.
            // В реальном приложении здесь может быть обработка ошибки.
            printf("Could not send to the queue.\n");
        } else {
            printf("Sent value: %ld\n", lValueToSend);
            lValueToSend++; // Увеличиваем значение для следующей отправки
        }
        
        // Пауза на 500 миллисекунд
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

// ----- ЗАДАЧА 2: ПОЛУЧАТЕЛЬ -----
// Эта задача получает данные из очереди
void vReceiverTask(void *pvParameters) {
    int32_t lReceivedValue;
    BaseType_t xStatus;
    const TickType_t xTicksToWait = pdMS_TO_TICKS(100); // Ждать 100 мс

    for (;;) {
        // Ожидаем данные в очереди.
        // portMAX_DELAY заставит задачу блокироваться (ждать) бесконечно,
        // если очередь пуста. Мы используем таймаут.
        xStatus = xQueueReceive(xQueue, &lReceivedValue, xTicksToWait);

        if (xStatus == pdPASS) {
            // Данные успешно получены
            printf("Received value: %ld\n", lReceivedValue);
        } else {
            // Данные не были получены в течение времени ожидания.
            // Задача может заняться чем-то другим.
            printf("Could not receive from the queue.\n");
        }
        // Эта задача не имеет явной задержки, т.к. ее выполнение
        // естественным образом блокируется функцией xQueueReceive.
    }
}

// ----- ИНИЦИАЛИЗАЦИЯ И ЗАПУСК -----
// Заглушка для инициализации железа (например, UART для printf)
void prvSetupHardware(void) {
    // Вставьте сюда код для инициализации вашего микроконтроллера.
    // Например, настройка системной частоты, GPIO, UART и т.д.
    printf("Hardware setup complete.\n");
}

int main(void) {
    // Инициализация специфичного для платформы оборудования
    prvSetupHardware();

    // Создаем очередь, которая может хранить 5 элементов типа int32_t.
    xQueue = xQueueCreate(5, sizeof(int32_t));

    if (xQueue != NULL) {
        // Создаем две задачи.
        xTaskCreate(vSenderTask,   // Указатель на функцию задачи
                    "Sender",      // Имя задачи (для отладки)
                    1000,          // Размер стека в словах
                    NULL,          // Параметры, передаваемые в задачу
                    1,             // Приоритет задачи
                    NULL);         // Хендл задачи (не используется)

        xTaskCreate(vReceiverTask,
                    "Receiver",
                    1000,
                    NULL,
                    2,             // У получателя приоритет выше
                    NULL);

        // Запускаем планировщик.
        vTaskStartScheduler();
    } else {
        // Не удалось создать очередь.
        printf("Queue could not be created.\n");
    }

    // Этот код никогда не должен выполниться,
    // так как управление передано планировщику FreeRTOS.
    for (;;);
    return 0;
}
