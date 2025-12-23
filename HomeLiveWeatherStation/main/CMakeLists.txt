idf_component_register(
    SRCS "main.c"
    SRC_DIRS "sensors"
    INCLUDE_DIRS "." "sensors"
    REQUIRES utils sensor_bme280 sensor_bh1750
)