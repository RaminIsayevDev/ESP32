#ifndef APP_MQTT_H
#define APP_MQTT_H

#include <stdbool.h>

void mqtt_app_start(void);
void mqtt_app_publish(const char *topic, const char *data, int qos, int retain);
int mqtt_app_publish_and_wait(const char *topic, const char *data, int qos, int retain, int timeout_ms);
bool mqtt_app_wait_connected(int timeout_ms);

#endif // APP_MQTT_H
