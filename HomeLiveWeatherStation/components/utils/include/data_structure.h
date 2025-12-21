#ifndef DATA_STRUCTURE_H
#define DATA_STRUCTURE_H

typedef struct {
    float temp;
    float humidity;
    float pressure;
    float lux;
    int co2_ppm;
} weather_data_t;

extern QueueHandle_t xWeatherQueue;            // Очередь данных
extern EventGroupHandle_t xWifiEventGroup;     // Флаг состояния сети

// Биты событий (флаги)
#define WIFI_CONNECTED_BIT BIT0
#define MQTT_CONNECTED_BIT BIT1

#endif