idf_component_register(SRCS "Amplifier_Init.c"
                    INCLUDE_DIRS "include"
                    REQUIRES All_I2C_Init esp_driver_i2s Audio_Code Display_Config)
