#include <stdio.h>
#include "stdint.h"


extern uint8_t amplifier_sts;


void I2s_Init(void);
void play_wav_8bit_array(const uint8_t *wav, uint32_t len);
void Audio_Stop_Wav(const uint8_t *wav);   /* stops that sound if it is playing (within ~10 ms) */

void Splash_Window_Audio(void);
void Birthday_Audio(void);
void Pomodoro_Audio(void);
void Angry_Audio_Play(void);
void Cable_Insert_Play(void);
void Cable_Remove_Play(void);
void Reminder_Audio_Play(void);
void Petting_Audio_Play(void);
void Reading_Audio_Play(void);

void Amplifier_Init (void);
void Amplifier_Active (uint8_t active);
void I2s_Deinit(void);