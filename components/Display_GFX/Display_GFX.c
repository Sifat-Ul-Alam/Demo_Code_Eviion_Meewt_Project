#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/spi_master.h"
#include "esp_log.h"
#include <string.h>
#include "esp_heap_caps.h"


#include "driver/gpio.h"
#include "Display_Config.h"
#include "soc/gpio_struct.h"
#include "Display_Init.h"




// Define the offsets based on supplier data
#define X_OFFSET 0
#if DISPLAY_DRIVER_NV3030B
#define Y_OFFSET 12 
#else
#define Y_OFFSET 0
#endif

// Define a static buffer in DMA-capable memory
// 240 pixels * 2 bytes = 480 bytes. This is very small.
static uint16_t line_buffer[240] __attribute__((aligned(4)));

void LCD_Fill_Rect_Fast(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    uint16_t x1 = x + X_OFFSET;
    uint16_t y1 = y + Y_OFFSET;
    uint16_t x2 = x1 + w - 1;
    uint16_t y2 = y1 + h - 1;

    // 1. Set Address Window
    LCD_command_WRITE(0x2A);
    LCD_data_WRITE(x1 >> 8);
    LCD_data_WRITE(x1 & 0xFF);
    LCD_data_WRITE(x2 >> 8);
    LCD_data_WRITE(x2 & 0xFF);

    LCD_command_WRITE(0x2B);
    LCD_data_WRITE(y1 >> 8);
    LCD_data_WRITE(y1 & 0xFF);
    LCD_data_WRITE(y2 >> 8);
    LCD_data_WRITE(y2 & 0xFF);

    LCD_command_WRITE(0x2C);

    // 2. Prepare the static buffer with the color (swap bytes for ST7789)
    uint16_t color_swapped = (color << 8) | (color >> 8);
    for (int i = 0; i < w && i < 240; i++)
    {
        line_buffer[i] = color_swapped;
    }

    // 3. Set up the SPI transaction once
    gpio_set_level(LCD_RS, 1); // Switch to Data mode

    spi_transaction_t t = {
        .length = w * 16, // length in bits
        .tx_buffer = line_buffer,
    };

    // 4. Send the buffer line-by-line
    // This uses DMA to blast the whole line to the screen at once
    for (int j = 0; j < h; j++)
    {
        spi_device_transmit(spi2, &t);
    }
}




#define FAST_CHUNK_SIZE 1024 
// Allocate this in DMA-capable memory
static uint16_t *pixel_buffer = NULL;

void Display_Buffers_Init() {
    if (pixel_buffer == NULL) {
        pixel_buffer = (uint16_t *)heap_caps_malloc(FAST_CHUNK_SIZE * sizeof(uint16_t), MALLOC_CAP_DMA);
    }
}


void LCD_Prepare_Image_Slot(uint16_t x, uint16_t y, uint16_t w, uint16_t h) {
    uint16_t x_start = x + X_OFFSET;
    uint16_t x_end   = x + X_OFFSET + w - 1;
    uint16_t y_start = y + Y_OFFSET;
    uint16_t y_end   = y + Y_OFFSET + h - 1;

    spi_transaction_t t;
    memset(&t, 0, sizeof(t)); // CRITICAL: Clear the struct

    // 1. Set Column Address (Command 0x2A)
    gpio_set_level(LCD_RS, 0); 
    t.length = 8;
    t.flags = SPI_TRANS_USE_TXDATA;
    t.tx_data[0] = 0x2A;
    spi_device_transmit(spi2, &t);

    gpio_set_level(LCD_RS, 1); // Data
    t.length = 32;
    t.tx_data[0] = (x_start >> 8);
    t.tx_data[1] = (x_start & 0xFF);
    t.tx_data[2] = (x_end >> 8);
    t.tx_data[3] = (x_end & 0xFF);
    spi_device_transmit(spi2, &t);

    // 2. Set Row Address (Command 0x2B)
    gpio_set_level(LCD_RS, 0);
    t.length = 8;
    t.tx_data[0] = 0x2B;
    spi_device_transmit(spi2, &t);

    gpio_set_level(LCD_RS, 1); // Data
    t.length = 32;
    t.tx_data[0] = (y_start >> 8);
    t.tx_data[1] = (y_start & 0xFF);
    t.tx_data[2] = (y_end >> 8);
    t.tx_data[3] = (y_end & 0xFF);
    spi_device_transmit(spi2, &t);
}





/**
 * @brief Draws an image at a specific coordinate without using DMA.
 * @param x          Starting X position (0 to 239)
 * @param y          Starting Y position (0 to 295)
 * @param w          Width of the image
 * @param h          Height of the image
 * @param data       Pointer to the image pixel data (uint16_t)
 */


void LCD_Draw_Image_At(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t *data)
{
    // --- ADJUST THESE OFFSETS IF YOUR IMAGE IS SHIFTED ---
    uint16_t x_offset = 0;
    #if DISPLAY_DRIVER_NV3030B
    uint16_t y_offset = 12; 
    #else
    uint16_t y_offset = 0;
    #endif

    uint16_t x_start = x + x_offset;
    uint16_t x_end = x + x_offset + w - 1;
    uint16_t y_start = y + y_offset;
    uint16_t y_end = y + y_offset + h - 1;

    // 1. Set Column Address (X-axis)
    uint8_t col_cmd = 0x2A;
    uint8_t col_data[] = {
        (x_start >> 8) & 0xFF, (x_start & 0xFF),
        (x_end >> 8) & 0xFF, (x_end & 0xFF)};

    gpio_set_level(LCD_RS, 0); // Command
    spi_transaction_t t_col_cmd = {.length = 8, .tx_buffer = &col_cmd};
    spi_device_transmit(spi2, &t_col_cmd);

    gpio_set_level(LCD_RS, 1); // Data
    spi_transaction_t t_col_data = {.length = 32, .tx_buffer = col_data};
    spi_device_transmit(spi2, &t_col_data);

    // 2. Set Row Address (Y-axis)
    uint8_t row_cmd = 0x2B;
    uint8_t row_data[] = {
        (y_start >> 8) & 0xFF, (y_start & 0xFF),
        (y_end >> 8) & 0xFF, (y_end & 0xFF)};

    gpio_set_level(LCD_RS, 0); // Command
    spi_transaction_t t_row_cmd = {.length = 8, .tx_buffer = &row_cmd};
    spi_device_transmit(spi2, &t_row_cmd);

    gpio_set_level(LCD_RS, 1); // Data
    spi_transaction_t t_row_data = {.length = 32, .tx_buffer = row_data};
    spi_device_transmit(spi2, &t_row_data);

    // 3. Write Memory Command (Start receiving pixels)
    uint8_t mem_write_cmd = 0x2C;
    gpio_set_level(LCD_RS, 0);
    spi_transaction_t t_mem = {.length = 8, .tx_buffer = &mem_write_cmd};
    spi_device_transmit(spi2, &t_mem);

    // 4. Send Image Data in Chunks (Maximum 64 bytes per SPI transfer)
    gpio_set_level(LCD_RS, 1);        // Switch to Data mode

    uint32_t total_pixels = w * h;
    uint16_t pixels_per_chunk = 2048; // 32 pixels * 2 bytes = 64 bytes

    for (uint32_t i = 0; i < total_pixels; i += pixels_per_chunk)
    {
        // Handle the last chunk if it's smaller than 32 pixels
        uint16_t current_chunk = (total_pixels - i < pixels_per_chunk) ? (total_pixels - i) : pixels_per_chunk;

        spi_transaction_t t = {
            .length = current_chunk * 16, // 16 bits per pixel
            .tx_buffer = &data[i],
        };
        spi_device_transmit(spi2, &t);
    }
}


void LCD_Draw_BitPacked_Data(uint32_t total_pixels, const uint8_t *packed_data, uint16_t bg_color, uint16_t color) {
    // Ensure buffer is initialized
    if (pixel_buffer == NULL) Display_Buffers_Init();

    // 1. Send Write Memory Command (0x2C)
    uint8_t mem_write_cmd = 0x2C;
    gpio_set_level(LCD_RS, 0);
    spi_transaction_t t_cmd = {
        .length = 8, 
        .flags = SPI_TRANS_USE_TXDATA, 
        .tx_data[0] = mem_write_cmd
    };
    spi_device_transmit(spi2, &t_cmd);

    gpio_set_level(LCD_RS, 1); // Switch to Data mode

    // 2. Handle Color Endianness safely
    // 0x55FB swapped becomes 0xFB55
    uint16_t color_on  = (uint16_t)((color >> 8) | (color << 8)); 
    uint16_t color_off = (uint16_t)((bg_color >> 8) | (bg_color << 8)); 

    uint32_t pixels_processed = 0;
    while (pixels_processed < total_pixels) {
        uint16_t current_chunk = (total_pixels - pixels_processed < FAST_CHUNK_SIZE) ? 
                                 (total_pixels - pixels_processed) : FAST_CHUNK_SIZE;

        // Fill DMA buffer
        for (uint16_t i = 0; i < current_chunk; i++) {
            uint32_t p = pixels_processed + i;
            // Check bit in packed data
            if (packed_data[p >> 3] & (1 << (7 - (p & 0x07)))) {
                pixel_buffer[i] = color_on;
            } else {
                pixel_buffer[i] = color_off;
            }
        }

        // 3. Transmit the chunk via DMA
        spi_transaction_t t_data = {
            .length = current_chunk * 16, 
            .tx_buffer = pixel_buffer
        };
        spi_device_transmit(spi2, &t_data);
        
        pixels_processed += current_chunk;
    }
}

/* Sends count ready-made pixels into the window set by LCD_Prepare_Image_Slot().
 * data: RGB565 byte-swapped like the image arrays, in DMA-capable (internal) RAM. */
void LCD_Draw_Pixels(uint32_t count, const uint16_t *data) {
    uint8_t mem_write_cmd = 0x2C;
    gpio_set_level(LCD_RS, 0);
    spi_transaction_t t_cmd = {
        .length = 8,
        .flags = SPI_TRANS_USE_TXDATA,
        .tx_data[0] = mem_write_cmd
    };
    spi_device_transmit(spi2, &t_cmd);

    gpio_set_level(LCD_RS, 1); // Data
    spi_transaction_t t_data = {
        .length = count * 16,
        .tx_buffer = data
    };
    spi_device_transmit(spi2, &t_data);
}

/* Like LCD_Draw_BitPacked_Data(), but 4 bits per pixel: each pixel is an index into
 * palette[16] (RGB565, normal byte order). Two pixels per byte, the first one in the high
 * nibble. Call LCD_Prepare_Image_Slot() first; total_pixels must be even. */
void LCD_Draw_Indexed4_Data(uint32_t total_pixels, const uint8_t *packed_data, const uint16_t *palette) {
    if (pixel_buffer == NULL) Display_Buffers_Init();

    uint16_t swapped[16];                       /* the colours in the byte order the panel wants */
    for (uint8_t i = 0; i < 16; i++) {
        swapped[i] = (uint16_t)((palette[i] >> 8) | (palette[i] << 8));
    }

    uint8_t mem_write_cmd = 0x2C;
    gpio_set_level(LCD_RS, 0);
    spi_transaction_t t_cmd = {
        .length = 8,
        .flags = SPI_TRANS_USE_TXDATA,
        .tx_data[0] = mem_write_cmd
    };
    spi_device_transmit(spi2, &t_cmd);

    gpio_set_level(LCD_RS, 1); // Data

    uint32_t pixels_processed = 0;
    while (pixels_processed < total_pixels) {
        uint16_t current_chunk = (total_pixels - pixels_processed < FAST_CHUNK_SIZE) ?
                                 (total_pixels - pixels_processed) : FAST_CHUNK_SIZE;

        const uint8_t *src = &packed_data[pixels_processed >> 1];
        for (uint16_t i = 0; i < current_chunk; i += 2) {
            uint8_t b = *src++;
            pixel_buffer[i]     = swapped[b >> 4];
            pixel_buffer[i + 1] = swapped[b & 0x0F];
        }

        spi_transaction_t t_data = {
            .length = current_chunk * 16,
            .tx_buffer = pixel_buffer
        };
        spi_device_transmit(spi2, &t_data);

        pixels_processed += current_chunk;
    }
}


/* =====================================================================
 * Compressed animation frames (run-length encoded, see the animation components)
 * ===================================================================== */

/* 1-bit frames: runs of (count 1..255, byte value). Unpacks src..end into dst (dst_len bytes). */
void Anim_Rle_Unpack(const uint8_t *src, const uint8_t *end, uint8_t *dst, uint32_t dst_len)
{
    uint32_t pos = 0;
    while (src + 1 < end && pos < dst_len) {
        uint8_t count = src[0], value = src[1];
        src += 2;
        if (count > dst_len - pos) {
            count = (uint8_t)(dst_len - pos);
        }
        memset(&dst[pos], value, count);
        pos += count;
    }
}

/* Colour (RGB565) frames: runs of (count 1..255, low byte, high byte) of the pixel words exactly as
 * the image arrays store them (byte-swapped). Same window and offsets as LCD_Draw_Image_At(); the
 * pixels are decoded chunk by chunk into the DMA buffer and sent, so no full-frame buffer is needed. */
void LCD_Draw_Image_Rle16_At(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t *src, const uint8_t *end)
{
    if (pixel_buffer == NULL) Display_Buffers_Init();

    uint16_t x_offset = 0;
    #if DISPLAY_DRIVER_NV3030B
    uint16_t y_offset = 12;
    #else
    uint16_t y_offset = 0;
    #endif

    uint16_t x_start = x + x_offset;
    uint16_t x_end = x + x_offset + w - 1;
    uint16_t y_start = y + y_offset;
    uint16_t y_end = y + y_offset + h - 1;

    uint8_t col_cmd = 0x2A;
    uint8_t col_data[] = {
        (x_start >> 8) & 0xFF, (x_start & 0xFF),
        (x_end >> 8) & 0xFF, (x_end & 0xFF)};
    gpio_set_level(LCD_RS, 0);
    spi_transaction_t t_col_cmd = {.length = 8, .tx_buffer = &col_cmd};
    spi_device_transmit(spi2, &t_col_cmd);
    gpio_set_level(LCD_RS, 1);
    spi_transaction_t t_col_data = {.length = 32, .tx_buffer = col_data};
    spi_device_transmit(spi2, &t_col_data);

    uint8_t row_cmd = 0x2B;
    uint8_t row_data[] = {
        (y_start >> 8) & 0xFF, (y_start & 0xFF),
        (y_end >> 8) & 0xFF, (y_end & 0xFF)};
    gpio_set_level(LCD_RS, 0);
    spi_transaction_t t_row_cmd = {.length = 8, .tx_buffer = &row_cmd};
    spi_device_transmit(spi2, &t_row_cmd);
    gpio_set_level(LCD_RS, 1);
    spi_transaction_t t_row_data = {.length = 32, .tx_buffer = row_data};
    spi_device_transmit(spi2, &t_row_data);

    uint8_t mem_write_cmd = 0x2C;
    gpio_set_level(LCD_RS, 0);
    spi_transaction_t t_mem = {.length = 8, .tx_buffer = &mem_write_cmd};
    spi_device_transmit(spi2, &t_mem);
    gpio_set_level(LCD_RS, 1);

    uint32_t left = (uint32_t)w * h;
    uint16_t run = 0;           /* pixels still owed by the current run */
    uint16_t word = 0;
    while (left > 0) {
        uint16_t chunk = (left < FAST_CHUNK_SIZE) ? (uint16_t)left : FAST_CHUNK_SIZE;
        for (uint16_t i = 0; i < chunk; i++) {
            if (run == 0) {
                if (src + 2 < end) {
                    run  = src[0];
                    word = (uint16_t)(src[1] | (src[2] << 8));
                    src += 3;
                } else {
                    run  = 0xFFFF;  /* data ran out: fill the rest with black rather than read past it */
                    word = 0;
                }
            }
            pixel_buffer[i] = word;
            run--;
        }
        spi_transaction_t t_data = {
            .length = chunk * 16,
            .tx_buffer = pixel_buffer
        };
        spi_device_transmit(spi2, &t_data);
        left -= chunk;
    }
}
