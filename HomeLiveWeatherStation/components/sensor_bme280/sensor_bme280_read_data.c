#include "bme280.h"
#include "bme280_defs.h"
#include "esp_log.h"
#include "sensor_bme280_init.h"

#include "sensor_bme280_read_data.h"

void sensor_bme280_read_data(struct bme280_dev *dev, struct bme280_data *comp_data) {
    int8_t rslt;
    uint32_t meas_delay;
    struct bme280_settings settings;

    /* 1. Получаем текущие настройки датчика */
    bme280_get_sensor_settings(&settings, dev);
    
    /* 2. Рассчитываем минимальную задержку, необходимую для измерения 
       на основе текущих настроек (оверсемплинга) */
    bme280_cal_meas_delay(&meas_delay, &settings);
    
    /* 3. Переводим датчик в Forced mode (он сделает одно измерение) */
    rslt = bme280_set_sensor_mode(BME280_POWERMODE_FORCED, dev);
    if (rslt != BME280_OK) {
        ESP_LOGE("BME280", "Failed to set forced mode");
        return;
    }
    
    /* 4. Ждем окончания измерения (важно!) */
    dev->delay_us(meas_delay, dev->intf_ptr);
    
    /* 5. Читаем результат */
    rslt = bme280_get_sensor_data(BME280_ALL, comp_data, dev);
    
    if (rslt == BME280_OK) {
        float p_mmHg = comp_data->pressure * 0.00750062;
        ESP_LOGI("BME280", "T: %.2f, H: %.2f, P: %.2f mmHg", 
                 comp_data->temperature, comp_data->humidity, p_mmHg);
    }
    
    // TODO: RETURNING BME280 DATA
}