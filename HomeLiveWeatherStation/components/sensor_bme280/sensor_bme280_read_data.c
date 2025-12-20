#include "bme280.h"
#include "bme280_defs.h"
#include "esp_log.h"
#include "sensor_bme280_init.h"

#include "sensor_bme280_read_data.h"

void sensor_bme280_read_data(struct bme280_dev *dev, struct bme280_data *comp_data) {
    // int8_t rslt;

    // // Передаем указатели напрямую, без &
    // rslt = bme280_get_sensor_data(BME280_ALL, comp_data, dev);

    // // Конвертация: Паскали -> мм рт. ст.
    // float pressure_mmHg = comp_data->pressure / 133.322387415;

    // if (rslt == BME280_OK) {
    //     // Используем -> для доступа к полям указателя
    //     ESP_LOGI("READ_DATA", "Temperature: %f °C", comp_data->temperature);
    //     ESP_LOGI("READ_DATA", "Pressure: %.2f mmHg", pressure_mmHg);     
    //     ESP_LOGI("READ_DATA", "Humidity: %f %%", comp_data->humidity);       
    // } else {
    //     ESP_LOGE("READ_DATA", "Failed to read sensor data (code %d)", rslt);
    // }

    int8_t rslt;
    uint32_t meas_delay;

    /* 1. Рассчитываем минимальную задержку, необходимую для измерения 
       на основе текущих настроек (оверсемплинга) */
    meas_delay = bme280_cal_meas_delay(&(dev->settings));
    /* 2. Переводим датчик в Forced mode (он сделает одно измерение) */
    rslt = bme280_set_sensor_mode(BME280_FORCED_MODE, dev);
    if (rslt != BME280_OK) {
        ESP_LOGE("BME280", "Failed to set forced mode");
        break;
    }
    /* 3. Ждем окончания измерения (важно!) */
    dev->delay_us(meas_delay, dev->intf_ptr);
    /* 4. Читаем результат */
    rslt = bme280_get_sensor_data(BME280_ALL, comp_data, dev);
    
    if (rslt == BME280_OK) {
        float p_mmHg = comp_data->pressure * 0.00750062;
        ESP_LOGI("BME280", "T: %.2f, H: %.2f, P: %.2f mmHg", 
                 comp_data->temperature, comp_data->humidity, p_mmHg);
    }
    /* 5. Ваша основная задержка (например, 10 секунд) */
    vTaskDelay(pdMS_TO_TICKS(10000));
}