idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES nvs_flash esp_wifi lwip esp-idf-ds3231 i2cdev)
