#ifndef myWifi_h
#define myWifi_h

#include "esp_event.h"
#include <stdint.h>

/* Event handler for Wi-Fi events */
void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);

/* Initializes Wi-Fi station mode, connects to AP */
void wifi_station_init(void);

#endif /* myWifi_h */