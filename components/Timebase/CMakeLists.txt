idf_component_register(SRCS "Timebase.c"
                    INCLUDE_DIRS "include"
                    REQUIRES esp_driver_gptimer Timebase_2 esp_timer)
