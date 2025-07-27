idf_component_register(SRCS "globals.c" "functions.c" "main.c"
                    INCLUDE_DIRS "." 
                    REQUIRES lvgl_esp32_drivers esp_lcd)
                    
set(EXTRA_COMPONENT_DIRS components/lvgl_esp32_drivers)