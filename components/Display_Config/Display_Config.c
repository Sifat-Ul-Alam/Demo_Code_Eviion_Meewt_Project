#include "driver/gpio.h"
#include "Display_Config.h"

void gpio_configure (void)
{
    esp_rom_gpio_pad_select_gpio(LCD_RST);
    esp_rom_gpio_pad_select_gpio(LCD_RS);
    esp_rom_gpio_pad_select_gpio(LCD_MOSI);

    gpio_reset_pin(LCD_RST);
    gpio_reset_pin(LCD_RS);
    gpio_reset_pin(LCD_MOSI);

    gpio_set_direction (LCD_RST, GPIO_MODE_OUTPUT);
    gpio_set_direction (LCD_RS, GPIO_MODE_OUTPUT);
    gpio_set_direction (LCD_MOSI, GPIO_MODE_OUTPUT);
};
