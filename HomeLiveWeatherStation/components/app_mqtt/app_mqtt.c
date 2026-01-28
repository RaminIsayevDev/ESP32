#include "esp_log.h"
#include "mqtt_client.h"
#include "app_mqtt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static const char *TAG = "MQTT_APP";

static esp_mqtt_client_handle_t client;
static EventGroupHandle_t s_mqtt_event_group;

#define MQTT_CONNECTED_BIT BIT0
#define MQTT_PUBLISHED_BIT BIT1

static int last_published_msg_id = -1;

static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
            xEventGroupSetBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_DISCONNECTED:
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
            xEventGroupClearBits(s_mqtt_event_group, MQTT_CONNECTED_BIT);
            break;
        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            if (event->msg_id == last_published_msg_id) {
                xEventGroupSetBits(s_mqtt_event_group, MQTT_PUBLISHED_BIT);
            }
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
            printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
            printf("DATA=%.*s\r\n", event->data_len, event->data);
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) {
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, (int)event_id);
    mqtt_event_handler_cb(event_data);
}

void mqtt_app_start(void)
{
    s_mqtt_event_group = xEventGroupCreate();

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = "mqtt://test.mosquitto.org",
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    esp_mqtt_client_start(client);
}

void mqtt_app_publish(const char *topic, const char *data, int qos, int retain)
{
    if (client) {
        esp_mqtt_client_publish(client, topic, data, 0, qos, retain);
    }
}

bool mqtt_app_wait_connected(int timeout_ms) {
    EventBits_t bits = xEventGroupWaitBits(s_mqtt_event_group, MQTT_CONNECTED_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(timeout_ms));
    return (bits & MQTT_CONNECTED_BIT) != 0;
}

int mqtt_app_publish_and_wait(const char *topic, const char *data, int qos, int retain, int timeout_ms) {
    if (!client) return -1;
    
    xEventGroupClearBits(s_mqtt_event_group, MQTT_PUBLISHED_BIT);
    last_published_msg_id = esp_mqtt_client_publish(client, topic, data, 0, qos, retain);
    
    if (last_published_msg_id < 0) {
        ESP_LOGE(TAG, "Failed to publish message");
        return -1;
    }

    EventBits_t bits = xEventGroupWaitBits(s_mqtt_event_group, MQTT_PUBLISHED_BIT, pdTRUE, pdTRUE, pdMS_TO_TICKS(timeout_ms));
    
    if (bits & MQTT_PUBLISHED_BIT) {
        return last_published_msg_id;
    } else {
        ESP_LOGW(TAG, "Publish timeout");
        return -1;
    }
}