#include "stdio.h"

typedef struct power_t{
  uint8_t system_boot;
  uint8_t Power_Button;
  uint8_t Power_Mode;
  uint8_t app_reboot;
  uint8_t factory_reset;
  
}power_t;
extern power_t Power;


void     Button_Struct_Init(void);
void     Power_Sequence_Struct_Init(void);
void     Power_Button_GPIO_Init(uint8_t mode);
void     Button_Response(uint8_t button);
void     Button_Check_Trigger(uint8_t button);
void     Button_Check_Press_Duration(uint32_t reference_time, uint8_t button);
void     Button_Scan(uint32_t reference_time);
uint8_t  Button_Short_Pressed(uint8_t button);
uint8_t  Button_Short_Pressed_Times(uint8_t times);   /* 1 once after exactly `times` (2, 3 ...) short presses within 3 s */
uint8_t  Button_Long_Pressed(uint8_t button);
uint8_t  get_Button_Response (uint8_t button);

uint8_t  Push_Button_Get_Reference_Timer_Use_Flag(void);
void     CHRG_En_GPIO_Init(uint8_t mode);
void     CHRG_Sts_GPIO_Init(uint8_t mode);

void     Battery_Charge_Enable (uint8_t val);
uint8_t  Get_Battery_Charge_Enable_Sts (void);
uint8_t  Get_Battery_Charge_Sts (void);
uint8_t  Get_Charger_Connect_Sts (void);
void     Charger_Connect_Sts_GPIO_Init(uint8_t mode);

void      Timer_Callback_4(void* arg);
uint64_t  ESP_Timer_4_Get_Time (void);
void      ESP_Timer_4_Start (void);
void      Reset_ESP_Timer_4(void);

