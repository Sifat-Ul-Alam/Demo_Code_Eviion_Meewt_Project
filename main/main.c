#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "driver/uart.h"
#include "driver/usb_serial_jtag.h"

#include "Display_Config.h"
#include "Amplifier_Init.h"
#include "All_I2C_Init.h"
#include "Display_Init.h"
#include "Display_GUI.h"
#include "Display_GFX.h"
#include "Color_code.h"
#include "Timebase_2.h"
#include "Audio_Code.h"
#include "Timebase.h"
#include "Flash.h"
#include "Power_Button.h"
#include "QMA6100P.h"


void app_main(void)
{
  Timebase_Init();
  Timebase_Init_2();
  QMA6100P_INT1_Gpio_Init();
  QMA6100P_Init();
  Amplifier_Init();
  gpio_configure();

  Spi_Init();
#if DISPLAY_DRIVER_NV3030B
  NV3030B_IPS_Initial();
#else
  ST7789P3_Init();
#endif
  Backlight_Init();

  Power_Button_GPIO_Init(2); // 1-input 2-output
  gpio_set_level(POWER_BUTTON, 1);
  gpio_set_level(POWER_CTRL, 0);

  Splash_Window();


  while (1)
  {
    qma_orientation_t confirmed = QMA6100P_Get_Confirmed_Orientation(2200);
    if(confirmed == QMA_ORIENT_LEFT_SIDE){
        printf("left side \n");
    }
    else if(confirmed == QMA_ORIENT_RIGHT_SIDE){
        printf("right side \n");
    }
    else if(confirmed == QMA_ORIENT_FLAT_UP){
        printf("Up side \n");
    }
    else if(confirmed == QMA_ORIENT_FLAT_DOWN){
        printf("Down side \n");
    }
    else if(confirmed == QMA_ORIENT_NORMAL){
        printf("Normal side \n");
    }
    else{

    }
    

    Timebase_Main_Loop_Executables();
    Timebase_Main_Loop_Executables_2();
  }
}
