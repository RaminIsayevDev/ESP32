#ifndef BH1750_H
#define BH1750_H

#include <stdint.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

/* BH1750 I2C Address */
#define BH1750_I2C_ADDR 0x23  // Default address when ADDR pin is LOW
// #define BH1750_I2C_ADDR 0x5C  // Alternative address when ADDR pin is HIGH

/* BH1750 Measurement Modes */
typedef enum {
    BH1750_CONTINUOUS_HIGH_RES = 0x10,  // Continuous H-resolution mode
    BH1750_CONTINUOUS_HIGH_RES2 = 0x11, // Continuous H-resolution mode 2
    BH1750_CONTINUOUS_LOW_RES = 0x13,   // Continuous L-resolution mode
    BH1750_ONE_TIME_HIGH_RES = 0x20,    // One time H-resolution mode
    BH1750_ONE_TIME_HIGH_RES2 = 0x21,   // One time H-resolution mode 2
    BH1750_ONE_TIME_LOW_RES = 0x23      // One time L-resolution mode
} bh1750_mode_t;

/* BH1750 Structure */
typedef struct {
    uint8_t i2c_addr;
    bh1750_mode_t mode;
    uint16_t light_level;  // Illuminance in lux
} bh1750_t;

/* Function prototypes */
esp_err_t bh1750_init(bh1750_t *dev, uint8_t i2c_num);
uint16_t bh1750_read_light_level(bh1750_t *dev, uint8_t i2c_num);
uint16_t bh1750_get_light_level(bh1750_t *dev);

#ifdef __cplusplus
}
#endif

#endif