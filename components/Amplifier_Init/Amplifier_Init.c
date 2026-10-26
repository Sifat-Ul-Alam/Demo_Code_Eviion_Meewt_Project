#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/i2s_std.h"
#include "esp_heap_caps.h"
#include "driver/i2c_master.h"

#include "Audio_Code.h"
#include "All_I2C_Init.h"
#include "Display_Config.h"
#include <math.h>


#define I2S_BCK_IO   11   
#define I2S_WS_IO    10   
#define I2S_DOUT_IO  12   
#define I2S_SD_MODE  13  


#define WAV_HEADER_SIZE 44
#define SAMPLE_RATE     44100   //44100
#define CHUNK_SAMPLES   256  


uint8_t amplifier_sts = 0;
void Amplifier_Active (uint8_t active){
    if(active == 1){
       gpio_set_level(I2S_SD_MODE, 1); 
       amplifier_sts = 1;
    }
    else{
       gpio_set_level(I2S_SD_MODE, 0); 
       amplifier_sts = 0;
    }
}



static i2s_chan_handle_t tx_chan;

void I2s_Init(void)
{
    i2s_chan_config_t chan_cfg = I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);

    i2s_new_channel(&chan_cfg, &tx_chan, NULL);

    i2s_std_config_t std_cfg = {
        .clk_cfg  = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = I2S_STD_PHILIPS_SLOT_DEFAULT_CONFIG(I2S_DATA_BIT_WIDTH_16BIT,I2S_SLOT_MODE_MONO),
        .gpio_cfg = {
            .bclk = I2S_BCK_IO,
            .ws   = I2S_WS_IO,
            .dout = I2S_DOUT_IO,
            .din  = I2S_GPIO_UNUSED
        }
    };

    i2s_channel_init_std_mode(tx_chan, &std_cfg);
    i2s_channel_enable(tx_chan);
}

void I2s_Deinit(void)
{
    // 1. Disable I2S channel
    ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));

    // 2. Delete I2S channel
    ESP_ERROR_CHECK(i2s_del_channel(tx_chan));

    tx_chan = NULL;
}

void Amplifier_Init (void){
    esp_rom_gpio_pad_select_gpio(I2S_SD_MODE);
    gpio_reset_pin(I2S_SD_MODE);
    gpio_set_direction (I2S_SD_MODE, GPIO_MODE_OUTPUT);

    I2s_Init();
}

/* Stop request for one sound: play_wav_8bit_array() checks it between chunks (256 samples) and ends early.
 * Only the sound whose array was named is stopped, other sounds are not affected. */
static const uint8_t * volatile s_stop_wav = NULL;

void Audio_Stop_Wav(const uint8_t *wav)
{
    s_stop_wav = wav;
}

void play_wav_8bit_array(const uint8_t *wav, uint32_t len)
{
    if (s_stop_wav == wav) {
        s_stop_wav = NULL;          /* an old stop request must not cut this new play */
    }
    int16_t pcm16[CHUNK_SAMPLES];
    size_t bytes_written;

    uint32_t index = WAV_HEADER_SIZE;

#if ALL_AUDIO_MUTE
    /* muted: the amplifier stays off and nothing is sent to I2S, but the sound still "takes its time"
     * (44.1 kHz, 8 bit) so every caller (splash, reading, tasks) keeps its timing; a stop request still works */
    while (index < len) {
        if (s_stop_wav == wav) {
            s_stop_wav = NULL;
            break;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        index += 441;
    }
    return;
#endif
    Amplifier_Active(1);
    while (index < len) {
        if (s_stop_wav == wav) {
            s_stop_wav = NULL;
            break;                  /* stopped from outside */
        }
        uint32_t samples = (len - index > CHUNK_SAMPLES)
                            ? CHUNK_SAMPLES
                            : (len - index);
        // 8-bit unsigned → 16-bit signed
        for (uint32_t i = 0; i < samples; i++){
            pcm16[i] = ((int16_t)wav[index + i] - 128) << 8;
        }
        i2s_channel_write(
            tx_chan,
            pcm16,
            samples * sizeof(int16_t),
            &bytes_written,
            portMAX_DELAY
        );
        index += samples;
    }
    Amplifier_Active(0);
}

void Splash_Window_Audio(void){
    play_wav_8bit_array(Splash_Audio, sizeof(Splash_Audio));   /* "Hi! ... I am Mute!" (Jenny, 44.1 kHz 8-bit) */ 
}

void Birthday_Audio(void){
    play_wav_8bit_array(Birthday_Tune, 621518); 
}

void Pomodoro_Audio(void){
    play_wav_8bit_array(Pomodoro_Data, 165128); 
}

void Angry_Audio_Play(void){
    play_wav_8bit_array(Angry_Audio, 55184); 
}
void Cable_Insert_Play(void){
    play_wav_8bit_array(Cable_Insert, 28948); 
}
void Cable_Remove_Play(void){
    play_wav_8bit_array(Cable_Remove, 15124); 
}
void Reminder_Audio_Play(void){
    play_wav_8bit_array(Reminder_Audio, 121464); 
}
void Petting_Audio_Play(void){
    play_wav_8bit_array(Petting_Audio, 19628); 
}
void Reading_Audio_Play(void){
    play_wav_8bit_array(Reading_Audio, 428066); 
}
