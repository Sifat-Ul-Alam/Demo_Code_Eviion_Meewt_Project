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

#include "driver/ledc.h"
#include "esp_err.h"
#include "Amplifier_Init.h"
#include "All_I2C_Init.h"

#define BACKLIGHT_GPIO          39
#define BACKLIGHT_LEDC_MODE     LEDC_LOW_SPEED_MODE
#define BACKLIGHT_LEDC_CHANNEL  LEDC_CHANNEL_0
#define BACKLIGHT_LEDC_TIMER    LEDC_TIMER_0
#define BACKLIGHT_DUTY_RES      LEDC_TIMER_8_BIT  // 8-bit resolution (0 to 255)
#define BACKLIGHT_FREQUENCY     5000              // 5 kHz frequency

/**
 * @brief Initializes the PWM channel for the TFT backlight
 */
void Backlight_Init(void)
{
    // 1. Configure the LEDC PWM Timer
    ledc_timer_config_t ledc_timer = {
        .speed_mode       = BACKLIGHT_LEDC_MODE,
        .timer_num        = BACKLIGHT_LEDC_TIMER,
        .duty_resolution  = BACKLIGHT_DUTY_RES,
        .freq_hz          = BACKLIGHT_FREQUENCY,
        .clk_cfg          = LEDC_AUTO_CLK
    };
    ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

    // 2. Configure the LEDC Channel mapping to GPIO 39
    ledc_channel_config_t ledc_channel = {
        .speed_mode     = BACKLIGHT_LEDC_MODE,
        .channel        = BACKLIGHT_LEDC_CHANNEL,
        .timer_sel      = BACKLIGHT_LEDC_TIMER,
        .intr_type      = LEDC_INTR_DISABLE,
        .gpio_num       = BACKLIGHT_GPIO,
        .duty           = 0, // Starts at 0% brightness (completely off)
        .hpoint         = 0
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
}

/**
 * @brief Sets the backlight brightness from 0 to 100%
 * @param percentage Brightness level (0 to 100)
 */
static uint8_t s_backlight_percent = 100;     /* the last level that was set (the backlight cannot be read back) */

uint8_t Backlight_Get_Brightness(void)
{
    return s_backlight_percent;
}

void Backlight_Set_Brightness(uint8_t percentage)
{
    if (percentage > 100) {
        percentage = 100;
    }
    s_backlight_percent = percentage;

    // Convert percentage (0-100) to 8-bit duty cycle value (0-255)
    uint32_t duty = (percentage * 255) / 100;

    // Apply the duty cycle
    ESP_ERROR_CHECK(ledc_set_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL, duty));
    
    // Update the hardware registers to apply the change
    ESP_ERROR_CHECK(ledc_update_duty(BACKLIGHT_LEDC_MODE, BACKLIGHT_LEDC_CHANNEL));
}

void Display_LED_Off(void){
   Backlight_Set_Brightness(0);
}

void Display_Sleep_Functions(void){
   Backlight_Set_Brightness(0);
}

void Display_LED_On_Smooth(void){
    for (int i = 0; i <= 100; i++) {
        Backlight_Set_Brightness(i);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}

void Display_LED_On(void){
    Backlight_Set_Brightness(100);
}

void Display_Dim_Functions(void){
    for (int i = 100; i >= 20; i--) {
        Backlight_Set_Brightness(i);
        vTaskDelay(pdMS_TO_TICKS(30));
    }
}



///--------------------------------SPI Interface wire--------------------------------------///
//////////////////////////////////////////////////////////////////////////////////////////////
spi_device_handle_t spi2;

void Spi_Init()
{
    esp_err_t ret;
    spi_bus_config_t buscfg = {
        .miso_io_num = -1,
        .mosi_io_num = LCD_MOSI, 
        .sclk_io_num = LCD_CLK,  
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 296 * 80 * sizeof(uint16_t),

    };
    ret = spi_bus_initialize(SPI2_HOST, &buscfg, SPI_DMA_CH_AUTO);
    ESP_ERROR_CHECK(ret);
    spi_device_interface_config_t devcfg = {
        .clock_speed_hz = 40 * 1000 * 1000,
        .mode = 0,
        .spics_io_num = LCD_CS,
        .queue_size = 5,
        .flags = SPI_DEVICE_HALFDUPLEX,
    };
    ESP_ERROR_CHECK(spi_bus_add_device(SPI2_HOST, &devcfg, &spi2));
};

void Spi_Deinit(void)
{
    // Must remove the device before freeing the bus
    ESP_ERROR_CHECK(spi_bus_remove_device(spi2));
    ESP_ERROR_CHECK(spi_bus_free(SPI2_HOST));
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
void LCD_command_WRITE(uint8_t command)
{
    gpio_set_level(LCD_RS, 0);         // DC Low for Command
    spi_transaction_t t = {
        .length = 8,                   // 8 bits
        .flags = SPI_TRANS_USE_TXDATA, // Use internal buffer
        .tx_data[0] = command,         // Put data directly in the struct
    };
    
    spi_device_transmit(spi2, &t);
}

void LCD_data_WRITE(uint8_t data)
{
    gpio_set_level(LCD_RS, 1); // DC High for Data
    spi_transaction_t t = {
        .length = 8,
        .flags = SPI_TRANS_USE_TXDATA,
        .tx_data[0] = data,
    };
    spi_device_transmit(spi2, &t);
}

////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////

void NV3030B_IPS_Initial(void)
{
gpio_set_level(LCD_RST, 1);
vTaskDelay(10 / portTICK_PERIOD_MS);
gpio_set_level(LCD_RST, 0);
vTaskDelay(50 / portTICK_PERIOD_MS); //200
gpio_set_level(LCD_RST, 1);
vTaskDelay(50 / portTICK_PERIOD_MS); //120

LCD_command_WRITE(0xfd);//private_access
LCD_data_WRITE(0x06);
LCD_data_WRITE(0x08);

////osc 11.2K 45.87M 	
//LCD_command_WRITE(0x60);//	
//LCD_data_WRITE(0x00);//osc_user_adj[3:0] 08
//LCD_data_WRITE(0x00);//
//LCD_data_WRITE(0x09);//01

LCD_command_WRITE(0x61);//	
LCD_data_WRITE(0x07);//
LCD_data_WRITE(0x07);//

LCD_command_WRITE(0x73);//
LCD_data_WRITE(0x70);//
LCD_command_WRITE(0x73);//
LCD_data_WRITE(0x00);//07

//bias
LCD_command_WRITE(0x62);//bias setting
LCD_data_WRITE(0x00);//01
LCD_data_WRITE(0x44);//04 44
LCD_data_WRITE(0x40);//44 65 40

LCD_command_WRITE(0x63);//
LCD_data_WRITE(0x41);//
LCD_data_WRITE(0x07);//
LCD_data_WRITE(0x12);//
LCD_data_WRITE(0x12);//

LCD_command_WRITE(0x64);//
LCD_data_WRITE(0x37);//

//VSP
LCD_command_WRITE(0x65);//Pump1=4.7MHz //PUMP1 VSP
LCD_data_WRITE(0x09);//D6-5:pump1_clk[1:0] clamp 28 2b
LCD_data_WRITE(0x10);//6.26
LCD_data_WRITE(0x21);

//VSN
LCD_command_WRITE(0x66); //pump=2 AVCL
LCD_data_WRITE(0x09); //clamp 08 0b 09
LCD_data_WRITE(0x10); //10
LCD_data_WRITE(0x21);

//add source_neg_time
LCD_command_WRITE(0x67);//pump_sel
LCD_data_WRITE(0x21);//21 20
LCD_data_WRITE(0x40);

//gamma vap/van
LCD_command_WRITE(0x68);//gamma vap/van
LCD_data_WRITE(0x9C);//90
LCD_data_WRITE(0x30);//30
LCD_data_WRITE(0x21);//17
LCD_data_WRITE(0x27);//

LCD_command_WRITE(0xb1);//frame rate
LCD_data_WRITE(0x0F);//0x0f fr_h[5:0] 0F
LCD_data_WRITE(0x02);//0x02 fr_v[4:0] 02
LCD_data_WRITE(0x02);//0x04 fr_div[2:0] 03

LCD_command_WRITE(0xB4);
LCD_data_WRITE(0x01); //00

////porch
LCD_command_WRITE(0xB5);
LCD_data_WRITE(0x02);//0x02 vfp[6:0]
LCD_data_WRITE(0x02);//0x02 vbp[6:0]
LCD_data_WRITE(0x0a);//0x0A hfp[6:0]
LCD_data_WRITE(0x14);//0x14 hbp[6:0]

LCD_command_WRITE(0xB6);
LCD_data_WRITE(0x04);//
LCD_data_WRITE(0x01);//
LCD_data_WRITE(0x9f);//
LCD_data_WRITE(0x00);//
LCD_data_WRITE(0x02);//

////gamme sel
LCD_command_WRITE(0xdf);//
LCD_data_WRITE(0x11);//gofc_gamma_en_sel=1

////gamma_test1 A1#_wangly
//3030b_gamma_new_12.18
//GAMMA---------------------------------/////////////

LCD_command_WRITE(0xE0);
LCD_data_WRITE(0x05);//pkp0[4:0] V3
LCD_data_WRITE(0x05);//pkp0[4:0] V3
LCD_data_WRITE(0x10);//pkp1[4:0] V7
LCD_data_WRITE(0x0E);//pkp2[4:0] V21
LCD_data_WRITE(0x10);//pkp3[4:0] V29 //
LCD_data_WRITE(0x0F);//pkp4[4:0] V37
LCD_data_WRITE(0x13);//pkp5[4:0] V45
LCD_data_WRITE(0x18);//pkp6[4:0] V56

LCD_command_WRITE(0xE3);
LCD_data_WRITE(0x18);//pkn0[4:0] V60
LCD_data_WRITE(0x13);//pkn1[4:0] V56
LCD_data_WRITE(0x0F);//pkn2[4:0] V45
LCD_data_WRITE(0x0F);//pkn3[4:0] V37
LCD_data_WRITE(0x10);//pkn4[4:0] V29 //
LCD_data_WRITE(0x09);//pkn5[4:0] V21
LCD_data_WRITE(0x07);//pkn6[4:0] V7
LCD_data_WRITE(0x05);//pkn7[4:0] V3

LCD_command_WRITE(0xE1);
LCD_data_WRITE(0x25);//prp0[6:0] V15
LCD_data_WRITE(0x6B);//prp1[6:0] V51

LCD_command_WRITE(0xE4);
LCD_data_WRITE(0x6C);//prn0[6:0] V51
LCD_data_WRITE(0x25);//prn1[6:0] V15

LCD_command_WRITE(0xE2);
LCD_data_WRITE(0x23);//vrp0[5:0] V0
LCD_data_WRITE(0x1D);//vrp1[5:0] V1
LCD_data_WRITE(0x1E);//vrp2[5:0] V2
LCD_data_WRITE(0x2F);//vrp3[5:0] V61
LCD_data_WRITE(0x30);//vrp4[5:0] V62
LCD_data_WRITE(0x3F);//vrp5[5:0] V63

LCD_command_WRITE(0xE5);
LCD_data_WRITE(0x3f);//vrn0[5:0] V63
LCD_data_WRITE(0x30);//vrn1[5:0] V62
LCD_data_WRITE(0x2F);//vrn2[5:0] V61
LCD_data_WRITE(0x1E);//vrn3[5:0] V2
LCD_data_WRITE(0x23);//vrn4[5:0] V1
LCD_data_WRITE(0x22);//vrn5[5:0] V0


//GAMMA---------------------------------/////////////
//source
LCD_command_WRITE(0xE6);
LCD_data_WRITE(0x00);
LCD_data_WRITE(0xff);//SC_EN_START[7:0] f0

LCD_command_WRITE(0xE7);
LCD_data_WRITE(0x01);//CS_START[3:0] 01
LCD_data_WRITE(0x04);//scdt_inv_sel cs_vp_en
LCD_data_WRITE(0x03);//CS1_WIDTH[7:0] 12
LCD_data_WRITE(0x03);//CS2_WIDTH[7:0] 12
LCD_data_WRITE(0x00);//PREC_START[7:0] 06
LCD_data_WRITE(0x12);//PREC_WIDTH[7:0] 12

LCD_command_WRITE(0xE8); //source
LCD_data_WRITE(0x00); //VCMP_OUT_EN 81-vcmp/vref_output pad
LCD_data_WRITE(0x70); //chopper_sel[6:4]
LCD_data_WRITE(0x00); //gchopper_sel[6:4] 60

////gate
LCD_command_WRITE(0xEc);
LCD_data_WRITE(0x52);//47 43

LCD_command_WRITE(0xF1);
LCD_data_WRITE(0x01);//te_pol tem_extend
LCD_data_WRITE(0x01);
LCD_data_WRITE(0x02);

LCD_command_WRITE(0xF6);
LCD_data_WRITE(0x01);
LCD_data_WRITE(0x30);
LCD_data_WRITE(0x00);
LCD_data_WRITE(0x00);

LCD_command_WRITE(0xfd);
LCD_data_WRITE(0xfa);
LCD_data_WRITE(0xfc);

LCD_command_WRITE(0x3a);
LCD_data_WRITE(0x05);//SH 0x66

LCD_command_WRITE(0x35);
LCD_data_WRITE(0x00);

LCD_command_WRITE(0x36);//bgr_[3]
LCD_data_WRITE(0x00);//c0

LCD_command_WRITE(0x11);
vTaskDelay(50/portTICK_PERIOD_MS); //200
LCD_command_WRITE(0x21);

LCD_command_WRITE(0x29); 
vTaskDelay(10/portTICK_PERIOD_MS); //10
}





// ST7789P3 2.01" IPS init - generated by Display Tuner on 2026-10-04
//
// Verified: every value below was read back from the ESP and matches what the panel displayed.
//
// Register writes are in the exact order the tuner last ran them on the panel
// (most recently changed last). Keep the order: some registers depend on it.
//
// Panel window: 240x296, X offset 0, Y offset 12
//   (add these to CASET/RASET when drawing).

#include <stdint.h>

static void lcd_write_data_buf(const uint8_t *buf, int len)
{
    for (int i = 0; i < len; i++) {
        LCD_data_WRITE(buf[i]);
    }
}

void ST7789P3_Init (void){
    gpio_set_level(LCD_RST, 1);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    gpio_set_level(LCD_RST, 0);
    vTaskDelay(800 / portTICK_PERIOD_MS);
    gpio_set_level(LCD_RST, 1);
    vTaskDelay(800 / portTICK_PERIOD_MS);


LCD_command_WRITE(0x11);     

vTaskDelay(120 / portTICK_PERIOD_MS);               

LCD_command_WRITE(0x36);     
LCD_data_WRITE(0x00);   

LCD_command_WRITE(0x21);     

LCD_command_WRITE(0xB2);     
LCD_data_WRITE(0x05);   
LCD_data_WRITE(0x05);   
LCD_data_WRITE(0x00);   
LCD_data_WRITE(0x33);   
LCD_data_WRITE(0x33);   

LCD_command_WRITE(0xB7);     
LCD_data_WRITE(0x75);   

LCD_command_WRITE(0xBB);     
LCD_data_WRITE(0x22);   

LCD_command_WRITE(0xC0);     
LCD_data_WRITE(0x2C);   

LCD_command_WRITE(0xC2);     
LCD_data_WRITE(0x01);   

LCD_command_WRITE(0xC3);     
LCD_data_WRITE(0x13);   

LCD_command_WRITE(0xC4);     
LCD_data_WRITE(0x20);   

LCD_command_WRITE(0xC6);     
LCD_data_WRITE(0x11);   

LCD_command_WRITE(0xD0);     
LCD_data_WRITE(0xA4);   
LCD_data_WRITE(0xA1);   

LCD_command_WRITE(0xD6);     
LCD_data_WRITE(0xA1);   

LCD_command_WRITE(0xE0);     
LCD_data_WRITE(0xD0);   
LCD_data_WRITE(0x05);   
LCD_data_WRITE(0x0A);   
LCD_data_WRITE(0x09);   
LCD_data_WRITE(0x08);   
LCD_data_WRITE(0x05);   
LCD_data_WRITE(0x2E);   
LCD_data_WRITE(0x44);   
LCD_data_WRITE(0x45);   
LCD_data_WRITE(0x0F);   
LCD_data_WRITE(0x17);   
LCD_data_WRITE(0x16);   
LCD_data_WRITE(0x2B);   
LCD_data_WRITE(0x33);   

LCD_command_WRITE(0xE1);     
LCD_data_WRITE(0xD0);   
LCD_data_WRITE(0x05);   
LCD_data_WRITE(0x0A);   
LCD_data_WRITE(0x09);   
LCD_data_WRITE(0x08);   
LCD_data_WRITE(0x05);   
LCD_data_WRITE(0x2E);   
LCD_data_WRITE(0x43);   
LCD_data_WRITE(0x45);   
LCD_data_WRITE(0x0F);   
LCD_data_WRITE(0x16);   
LCD_data_WRITE(0x16);   
LCD_data_WRITE(0x2B);   
LCD_data_WRITE(0x33); 

LCD_command_WRITE(0x3A); 
LCD_data_WRITE(0x05);  

LCD_command_WRITE(0x29);    
vTaskDelay(200 / portTICK_PERIOD_MS);           
}

/*
void ST7789P3_Init(void)
{
    gpio_set_level(LCD_RST, 1);
    vTaskDelay(200 / portTICK_PERIOD_MS);
    gpio_set_level(LCD_RST, 0);
    vTaskDelay(800 / portTICK_PERIOD_MS);
    gpio_set_level(LCD_RST, 1);
    vTaskDelay(800 / portTICK_PERIOD_MS);

    LCD_command_WRITE(0x11); // Sleep out
    vTaskDelay(120 / portTICK_PERIOD_MS);

    LCD_command_WRITE(0x3A); LCD_data_WRITE(0x55); // COLMOD: RGB565
    LCD_command_WRITE(0xC2); LCD_data_WRITE(0x01); // VRH from command
    LCD_command_WRITE(0xC4); LCD_data_WRITE(0x20); // kept from the vendor init
    LCD_command_WRITE(0xD6); LCD_data_WRITE(0xA1);
    LCD_command_WRITE(0xB2); // PORCTRL
    {
        static const uint8_t d[] = {
            0x05, 0x05, 0x00, 0x33, 0x33
        };
        lcd_write_data_buf(d, sizeof(d));
    }
    LCD_command_WRITE(0xB7); LCD_data_WRITE(0x75); // GCTRL
    LCD_command_WRITE(0xBB); LCD_data_WRITE(0x2D); // VCOMS
    LCD_command_WRITE(0xC0); LCD_data_WRITE(0x2C); // LCMCTRL
    LCD_command_WRITE(0xC5); LCD_data_WRITE(0x20); // VCMOFSET
    LCD_command_WRITE(0xC6); LCD_data_WRITE(0x11); // FRCTRL2
    LCD_command_WRITE(0xD0); // PWCTRL1
    {
        static const uint8_t d[] = {
            0xA4, 0xA1
        };
        lcd_write_data_buf(d, sizeof(d));
    }
    LCD_command_WRITE(0xE9); // EQCTRL
    {
        static const uint8_t d[] = {
            0x0E, 0x0E, 0x07
        };
        lcd_write_data_buf(d, sizeof(d));
    }
    LCD_command_WRITE(0xE0); // PVGAMCTRL
    {
        static const uint8_t d[] = {
            0xD0, 0x04, 0x0A, 0x0C, 0x0E, 0x2A, 0x40, 0x65,
            0x51, 0x18, 0x0D, 0x0B, 0x1F, 0x23
        };
        lcd_write_data_buf(d, sizeof(d));
    }
    LCD_command_WRITE(0xE1); // NVGAMCTRL
    {
        static const uint8_t d[] = {
            0xD0, 0x04, 0x09, 0x0C, 0x0E, 0x2B, 0x40, 0x55,
            0x56, 0x2F, 0x1F, 0x1F, 0x20, 0x23
        };
        lcd_write_data_buf(d, sizeof(d));
    }
    LCD_command_WRITE(0x36); LCD_data_WRITE(0x00); // MADCTL
    LCD_command_WRITE(0xE2); // DGMLUTR
    {
        static const uint8_t d[] = {
            0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C,
            0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C,
            0x40, 0x44, 0x48, 0x4C, 0x50, 0x54, 0x58, 0x5C,
            0x60, 0x64, 0x68, 0x6C, 0x70, 0x74, 0x78, 0x7C,
            0x80, 0x84, 0x88, 0x8C, 0x90, 0x94, 0x98, 0x9C,
            0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
            0xC0, 0xC4, 0xC8, 0xCC, 0xD0, 0xD4, 0xD8, 0xDC,
            0xE0, 0xE4, 0xE8, 0xEC, 0xF0, 0xF4, 0xF8, 0xFC
        };
        lcd_write_data_buf(d, sizeof(d));
    }
    LCD_command_WRITE(0xE3); // DGMLUTB
    {
        static const uint8_t d[] = {
            0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C,
            0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C,
            0x40, 0x44, 0x48, 0x4C, 0x50, 0x54, 0x58, 0x5C,
            0x60, 0x64, 0x68, 0x6C, 0x70, 0x74, 0x78, 0x7C,
            0x80, 0x84, 0x88, 0x8C, 0x90, 0x94, 0x98, 0x9C,
            0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
            0xC0, 0xC4, 0xC8, 0xCC, 0xD0, 0xD4, 0xD8, 0xDC,
            0xE0, 0xE4, 0xE8, 0xEC, 0xF0, 0xF4, 0xF8, 0xFC
        };
        lcd_write_data_buf(d, sizeof(d));
    }
    LCD_command_WRITE(0xBA); LCD_data_WRITE(0x00); // DGMEN
    LCD_command_WRITE(0x21); // Inversion ON
    LCD_command_WRITE(0x55); LCD_data_WRITE(0x80); // WRCACE
    LCD_command_WRITE(0x26); LCD_data_WRITE(0x01); // GAMSET
    LCD_command_WRITE(0xC3); LCD_data_WRITE(0x1D); // VRHS


    LCD_command_WRITE(0x29); // Display ON
    vTaskDelay(200 / portTICK_PERIOD_MS);
}

*/



void Display_Power_Down(void)
{
    //Stop driving any signal into the display IC before cutting its supply
    gpio_set_level(LCD_CS, 0);
    gpio_set_level(LCD_RST, 0);
    gpio_set_level(LCD_RS, 0);
    Amplifier_Active(0);

    gpio_set_level(POWER_CTRL, 1);   // now safe to cut VDD
}


