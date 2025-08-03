idf_component_register(SRCS "main.c" "tasks.c" "globals.c"
                       INCLUDE_DIRS "../include"
                       REQUIRES "lvgl-9.3.0" "esp-idf-ds3231" "esp_driver_spi" "esp_wifi" "nvs_flash"
                       PRIV_REQUIRES "lwip")