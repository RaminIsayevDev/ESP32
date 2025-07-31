idf_component_register(SRCS "main.c" "globals.c" "tasks.c"
                       INCLUDE_DIRS "."
                       REQUIRES esp_wifi nvs_flash esp_driver_spi)
