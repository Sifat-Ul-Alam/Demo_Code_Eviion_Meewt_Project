idf_component_register(SRCS "Display_Init.c"
                    INCLUDE_DIRS "include"
                    REQUIRES Amplifier_Init esp_driver_ledc Display_Config esp_lcd)
