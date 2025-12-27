#ifndef APP_MQTT_H
#define APP_MQTT_H

void mqtt_app_start(void);
void mqtt_app_publish(const char *topic, const char *data);

#endif // APP_MQTT_H
