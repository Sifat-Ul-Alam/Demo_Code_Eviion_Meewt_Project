idf_component_register(SRCS "Display_GUI.c"
                    INCLUDE_DIRS "include"
                    REQUIRES Amplifier_Init Display_GFX BG_Image esp_timer Display_Init)

