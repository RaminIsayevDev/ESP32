idf_component_register(
    SRCS "main.c" "sensors/task_sensors.c"
    INCLUDE_DIRS "." "sensors"
    REQUIRES utils sensor_bme280 sensor_bh1750 sensor_mq135 wifi_manager json mqtt app_mqtt
)