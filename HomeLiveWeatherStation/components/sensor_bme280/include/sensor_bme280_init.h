#ifndef SENSOR_BME280_INIT_H
#define SENSOR_BME280_INIT_H

#include "esp_log.h"
#include "bme280.h"
#include "bme280_defs.h"
#include "driver/i2c.h"
#include "esp_system.h"

// Wrapped function sensor initializing
esp_err_t bme280_sensor_init(void);

// Глобальная переменная для устройства BME280
extern struct bme280_dev bme280_dev;

// global tool-functions
int8_t bme280_i2c_read(uint8_t reg_addr, uint8_t *reg_data, uint32_t len, void *intf_ptr);
int8_t bme280_i2c_write(uint8_t reg_addr, const uint8_t *reg_data, uint32_t len, void *intf_ptr);
void bme280_delay_us(uint32_t period, void *intf_ptr);

#endif