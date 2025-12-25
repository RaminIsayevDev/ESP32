idf_component_register(SRCS "sensor_mq135_init.c" "sensor_mq135.c"
                       INCLUDE_DIRS "include"
                       REQUIRES driver esp_adc esp_timer)