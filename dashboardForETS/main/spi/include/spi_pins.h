#ifndef SPI_PINS_H
#define SPI_PINS_H

#include <driver/gpio.h>
#include <driver/spi_master.h>

#define LCD_HOST    SPI2_HOST
#define LCD_H_RES   128
#define LCD_V_RES   160
#define PIN_NUM_MISO -1  // Для дисплея чтение не обязательно
#define PIN_NUM_MOSI 23
#define PIN_NUM_CLK  18
#define PIN_NUM_CS   5
#define PIN_NUM_DC   2
#define PIN_NUM_RST  4

#endif // SPI_PINS_H
