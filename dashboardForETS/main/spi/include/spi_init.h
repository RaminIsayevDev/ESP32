#ifndef SPI_INIT_H
#define SPI_INIT_H
#include "st7735s.h"

esp_err_t spi_init();
void my_spi_write_cmd(uint8_t cmd);
void my_spi_write_data(uint8_t data);

#endif // SPI_INIT_H  