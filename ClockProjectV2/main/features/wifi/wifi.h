#ifndef WIFI.H
#define WIFI.H

void my_wifi_connected_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void my_wifi_started_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void my_wifi_disconnected_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data);
void my_wifi_ip_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void wifi_task(void *pvParameters);

#endif