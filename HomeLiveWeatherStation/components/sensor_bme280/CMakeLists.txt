idf_component_register(SRCS "sensor_bme280_init.c" "sensor_bme280_read_data.c" "bme280.c"
                       INCLUDE_DIRS "include"
                       REQUIRES driver)