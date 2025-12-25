#include "sensor_bme280_read_data.h"
#include "bme280.h"
#include "bme280_defs.h"
#include "esp_log.h"
#include "sensor_bme280_init.h"
#include <string.h>

static const char *TAG = "BME280_READ";

struct bme280_data sensor_bme280_read_data(struct bme280_dev *dev) {
    int8_t rslt;
    uint32_t meas_delay;
    struct bme280_settings settings;
    struct bme280_data comp_data;

    // Initialize data structure to zero
    memset(&comp_data, 0, sizeof(struct bme280_data));

    /* 1. Get current sensor settings */
    rslt = bme280_get_sensor_settings(&settings, dev);
    if (rslt != BME280_OK) {
        ESP_LOGE(TAG, "Failed to get sensor settings (code %+d)", rslt);
        return comp_data; // Return zeroed data on error
    }

    /* 2. Calculate the minimum measurement delay */
    bme280_cal_meas_delay(&meas_delay, &settings);

    /* 3. Set sensor to Forced mode (single measurement) */
    rslt = bme280_set_sensor_mode(BME280_POWERMODE_FORCED, dev);
    if (rslt != BME280_OK) {
        ESP_LOGE(TAG, "Failed to set forced mode (code %+d)", rslt);
        return comp_data; // Return zeroed data on error
    }

    /* 4. Wait for the measurement to complete */
    dev->delay_us(meas_delay, dev->intf_ptr);

    /* 5. Read the sensor data */
    rslt = bme280_get_sensor_data(BME280_ALL, &comp_data, dev);

    if (rslt == BME280_OK) {
        // Convert pressure from Pa to mmHg and store it back in the structure
        float p_mmHg = comp_data.pressure * 0.00750062;
        comp_data.pressure = p_mmHg;
    } else {
        ESP_LOGE(TAG, "Failed to get sensor data (code %+d)", rslt);
        // Clear data on failure to ensure no stale values are returned
        memset(&comp_data, 0, sizeof(struct bme280_data));
    }

    return comp_data;
}
