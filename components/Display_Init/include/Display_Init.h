#include "stdio.h"
#include "stdint.h"
#include "driver/spi_master.h"


extern spi_device_handle_t spi2;


void LCD_INIT(void);
void ST7789P3_Init (void);
void Spi_Init();
void LCD_data_WRITE (uint8_t data);
void LCD_command_WRITE (uint8_t command);

void NV3030B_IPS_Initial(void);

void Backlight_Init(void);
void Backlight_Set_Brightness(uint8_t percentage);
uint8_t Backlight_Get_Brightness(void);   /* the level that was last set, 0..100 */


void Display_LED_Off(void);
void Display_Sleep_Functions(void);
void Display_LED_On(void);
void Display_LED_On_Smooth(void);
void Display_Dim_Functions(void);

void Spi_Deinit(void);

void Display_Power_Down(void);