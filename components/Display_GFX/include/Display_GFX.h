#include "stdio.h"
#include "stdint.h"


void LCD_Fill_Rect_Fast(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) ;
void LCD_Prepare_Image_Slot(uint16_t x, uint16_t y, uint16_t w, uint16_t h);
void LCD_Draw_BitPacked_Data(uint32_t total_pixels, const uint8_t *packed_data, uint16_t bg_color, uint16_t color);
void LCD_Draw_Indexed4_Data(uint32_t total_pixels, const uint8_t *packed_data, const uint16_t *palette);   /* 4 bits per pixel, palette[16] */
void LCD_Draw_Pixels(uint32_t count, const uint16_t *data);   /* raw byte-swapped RGB565 into the window from LCD_Prepare_Image_Slot() */
void LCD_Draw_Image_At(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data);
void Anim_Rle_Unpack(const uint8_t *src, const uint8_t *end, uint8_t *dst, uint32_t dst_len);   /* 1-bit animation frames */
void LCD_Draw_Image_Rle16_At(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *src, const uint8_t *end);   /* RGB565 run-length frames */

