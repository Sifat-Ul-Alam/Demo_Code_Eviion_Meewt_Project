idf_component_register(SRCS "Power_Button.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_gpio Display_Config esp_timer)
