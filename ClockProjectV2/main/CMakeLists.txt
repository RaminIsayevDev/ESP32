idf_component_register(SRCS "main.c" "features/wifi/wifi.c" "features/sntp/sntp.c" "features/rtc/rtc.c" "features/display/display.c"
                       INCLUDE_DIRS "."
                       REQUIRES nvs_flash esp_wifi tm1637)