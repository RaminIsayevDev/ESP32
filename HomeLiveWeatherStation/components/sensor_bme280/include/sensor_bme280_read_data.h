#ifndef SENSOR_BME280_READ_DATA_H
#define SENSOR_BME280_READ_DATA_H

#include "bme280.h"

extern struct bme280_data comp_data;

void sensor_bme280_read_data(struct bme280_dev *dev, struct bme280_data *comp_data);

#endif