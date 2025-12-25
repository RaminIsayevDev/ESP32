#ifndef I2C_BUS_H
#define I2C_BUS_H

#include "esp_err.h"

#define I2C_MASTER_SCL_IO 22
#define I2C_MASTER_SDA_IO 21
#define I2C_MASTER_NUM 0
#define I2C_MASTER_FREQ_HZ 100000

esp_err_t i2c_master_init(void);

#endif