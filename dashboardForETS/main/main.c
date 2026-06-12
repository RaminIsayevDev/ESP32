#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <driver/gpio.h>
#include <driver/spi_master.h>
#include "spi/include/spi_init.h"
#include "display/include/gui_task.h"

void app_main(void)
{
    // Сначала нам нужно инициализировать SPI
    init_spi_bus();

    xTaskCreate(gui_task, "gui_task", 4096, NULL, 5, NULL);
}
