#include <stdio.h>
#include "stdint.h"

#define CODE_VERIFICATION_PRINT  1
#define FLASH_READ_DATA_DEBUG    1
#define DISPLAY_DRIVER_NV3030B   1

/* 1 = ALL audio is muted (nothing comes out of the speaker), 0 = audio on.
 * Sounds still "take their time" when muted, so animations and screens keep the same timing. */
#define ALL_AUDIO_MUTE           0

#define ST7789P3_WIDTH           296
#define ST7789P3_HEIGHT          240
#define ST7789P3_PIXEL_COUNT	 ST7789P3_WIDTH * ST7789P3_HEIGHT
 
// GPIO X
#define LCD_RS        2
#define LCD_RST       1 
#define LCD_MOSI      42 
#define LCD_CLK       41
#define LCD_CS        40

#define POWER_CTRL    38   
#define POWER_BUTTON  7  

void    gpio_configure (void);