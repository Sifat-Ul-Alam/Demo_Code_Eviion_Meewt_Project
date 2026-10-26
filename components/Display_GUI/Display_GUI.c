#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_err.h"
#include "esp_log.h"

#include "esp_sleep.h"
#include <string.h>

#include "Display_Init.h"
#include "BG_Image.h"
#include "Display_GFX.h"
#include "Amplifier_Init.h"



void Splash_Window (void){
  Display_LED_Off();
  LCD_Fill_Rect_Fast(0,0,240,296,0x0000);
  LCD_Draw_Image_At(0,0,240,296,EViiON_BG_Image_296x240);  
  Display_LED_On_Smooth();

  Cable_Insert_Play();
}

