#include <stdio.h>
#include "stdint.h"


extern  uint64_t read_value_1;


void    Display_Dim_Set_Flag(uint8_t value);
void    Display_Dim_Clear_Flag(uint8_t value);
uint8_t Get_Display_Dim_Flag(uint8_t value);

void    Display_Sleep_Set_Flag(uint8_t value);
void    Display_Sleep_Clear_Flag(uint8_t value);
uint8_t Get_Display_Sleep_Flag(uint8_t value);

void    Display_Light_Sleep_Set_Flag(uint8_t value);
void    Display_Light_Sleep_Clear_Flag(uint8_t value);
uint8_t Get_Display_Light_Sleep_Flag(uint8_t value);

void     Flash_Parameter_Struct_Init(void);
uint64_t Flash_Write_Data(void);
void     Current_Parameter_Set_Pomodoro_Minute_Sts(uint8_t val);
uint8_t  Current_Parameter_Get_Pomodoro_Minute_Sts(void);
void     Current_Parameter_Set_System_Reset_Sts(uint8_t val);
uint8_t  Current_Parameter_Get_System_Reset_Sts(void);
void     Current_Parameter_Set_System_Restart_Sts(uint8_t val);
uint8_t  Current_Parameter_Get_System_Restart_Sts(void);

void    Splash_Window (void);

void    Battery_Status_Update (void);

void    Display_Parameter_Struct_Init(void);
void    Display_Brightness_Control_Fuctions (void);

void    Display_Wakeup_Routine(void);
uint8_t Display_Wakeup_Sequence(void);
void    Display_Wakeup_Function(void);

void    Pomodoro_window (void);
void    wake_display_hw(void);
void    Stop_All_Features(void);
void    Power_Window(uint8_t selected);   /* power menu: Restart | Shutdown, selected = POWER_SELECT_RESTART / POWER_SELECT_SHUTDOWN */
void    Unpaired_Function(void);   /* not paired yet: QR / wake-up loop + pairing animation; call every main-loop pass */