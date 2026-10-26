#include "stdio.h"

void     Timebase_Struct_Init_2(void);
uint8_t  Timebase_Token_Executing_2(void);
void     Timebase_Token_Add_2(void);
void     Timebase_Token_Remove_2(void);
void     Timebase_Token_Remove_All_2(void);


uint32_t Timebase_Timer_Get_SubSeconds_2(void);
uint32_t Timebase_Timer_Get_Seconds_2(void);
void     Timebase_Timer_Set_SubSeconds_2(uint32_t value);
void     Timebase_Timer_Set_Seconds_2(uint32_t value);
void     Timebase_Timer_Delay_SubSeconds_2(uint16_t value);
void     Timebase_Timer_Delay_Seconds_2(uint16_t value);


uint8_t  Timebase_DownCounter_Get_Status_2(uint8_t window);
void     Timebase_DownCounter_Set_Status_2(uint8_t window, uint8_t value);
int32_t  Timebase_DownCounter_Get_Value_2(uint8_t window);
void     Timebase_DownCounter_Set_Value_2(uint8_t window, int32_t value);
uint32_t Timebase_DownCounter_Get_EndValue_2(uint8_t window);
void     Timebase_DownCounter_Set_EndValue_2(uint8_t window, uint32_t value);
uint32_t Timebase_DownCounter_Get_PeriodValue_2(uint8_t window);
void     Timebase_DownCounter_Set_PeriodValue_2(uint8_t window, int32_t value);
uint8_t  Timebase_DownCounter_Get_Period_Flag_2(uint8_t window);
void     Timebase_DownCounter_Set_Period_Flag_2(uint8_t window);
void     Timebase_DownCounter_Clear_Period_Flag_2(uint8_t window);

void     Timebase_DownCounter_Reset_2(uint8_t window);
void     Timebase_DownCounter_Clear_All_Flags_2(uint8_t window);
void     Timebase_DownCounter_Start_2(uint8_t window);
void     Timebase_DownCounter_Stop_2(uint8_t window);
void     Timebase_DownCounter_Set_Securely_2(uint8_t window, uint32_t value);
void     Timebase_DownCounter_Set_Forcefully_2(uint8_t window, uint32_t value);
void     Timebase_DownCounter_Update_2(uint8_t window);
uint8_t  Timebase_DownCounter_Expired_2(uint8_t window);
uint8_t  Timebase_DownCounter_Expired_Event_2(uint8_t window);

void     Timebase_DownCounter_Set_Period_Value_Securely_2(uint8_t window, uint32_t value);
uint32_t Timebase_DownCounter_Get_Remaining_Period_Value_2(uint8_t window);
uint8_t  Timebase_DownCounter_Period_Value_Expired_2(uint8_t window);
uint8_t  Timebase_DownCounter_Period_Value_Expired_Event_2(uint8_t window);
void     Timebase_DownCounter_Update_All_2(void);
void     Timebase_DownCounter_Reset_All_2(void);
void     Timebase_Reset_2(void);
void     Timebase_Main_Loop_Executables_2(void);
void     Timebase_ISR_Executables_2(void);

void     Timebase_Init_2(void);

