#include "task_sensors.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

// Задача датчиков
static void sensors_task(void *arg) {
    // TODO Сделать саму задачу работы датчиков
}

void sensors_init(void) {
    xTaskCreate(
        sensors_task,
        "sensors_task",
        // TODO доделать модуль инициализации задачи
    );
}