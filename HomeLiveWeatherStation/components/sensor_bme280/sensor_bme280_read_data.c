#include "bme280.h"
#include "bme280_defs.h"
#include "esp_log.h"
#include "sensor_bme280_init.h"

#include "sensor_bme280_read_data.h"

void sensor_bme280_read_data(struct bme280_dev *dev, struct bme280_data *comp_data) {
    int8_t rslt;

    // Передаем указатели напрямую, без &
    rslt = bme280_get_sensor_data(BME280_ALL, comp_data, dev);

    // Конвертация: Паскали -> мм рт. ст.
    float pressure_mmHg = comp_data->pressure / 133.322387415;

    if (rslt == BME280_OK) {
        // Используем -> для доступа к полям указателя
        ESP_LOGI("READ_DATA", "Temperature: %f °C", comp_data->temperature);
        ESP_LOGI("READ_DATA", "Pressure: %.2f mmHg", pressure_mmHg);     
        ESP_LOGI("READ_DATA", "Humidity: %f %%", comp_data->humidity);       
    } else {
        ESP_LOGE("READ_DATA", "Failed to read sensor data (code %d)", rslt);
    }
}