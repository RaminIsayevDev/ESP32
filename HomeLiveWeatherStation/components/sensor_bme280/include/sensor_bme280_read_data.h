#ifndef SENSOR_BME280_READ_DATA_H
#define SENSOR_BME280_READ_DATA_H

#include "bme280.h"
#include "bme280_defs.h"

struct bme280_data sensor_bme280_read_data(struct bme280_dev *dev);

#endif