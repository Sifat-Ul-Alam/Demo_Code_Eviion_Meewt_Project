#include "freertos/FreeRTOS.h"
#include "driver/gptimer.h"
#include "freertos/queue.h"
#include "stdio.h"

bool     Gp_Timer_Alarm(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data);
void     GpTimer_Init_Dependent(gptimer_handle_t *gptimer);
void     GpTimer_Init_Independent(gptimer_handle_t *gptimer, uint64_t resolution);
void     GpTimer_Alarm_Set(gptimer_handle_t *gptimer, QueueHandle_t *queue, uint64_t alarm_time);
void     GpTimer_Alarm_Reset_Auto_Reload(gptimer_handle_t *gptimer,uint64_t alarm_time);
void     GpTimer_Alarm_Reset_Run_Time(gptimer_handle_t gptimer,uint64_t value);
uint64_t Get_GpTimer_Run_Time(gptimer_handle_t gptimer);

void     Def_Timer_Start(void);
void     Def_Timer_Reset(void);
uint8_t  Get_Def_Timer_Complete_Flag (void);
uint64_t Get_Def_Timer_Run_Time (void);

void     Timer_Callback_2(void* arg);
uint64_t ESP_Timer_2_Get_Time (void);
void     ESP_Timer_2_Start (void);
void     Reset_ESP_Timer_2(void);

void     ComPort_UartData_Update_Timer_Start(void);
void     ComPort_UartData_Update_Timer_Reset(void);
uint8_t  Get_ComPort_UartData_Timer_Complete_Flag (void);
//Config Functions
void     Timebase_Struct_Init(void);
//Token Functions
uint8_t  Timebase_Token_Executing(void);
void     Timebase_Token_Add(void);
void     Timebase_Token_Remove(void);
void     Timebase_Token_Remove_All(void);

//Timer Functions
uint32_t Timebase_Timer_Get_SubSeconds(void);
uint32_t Timebase_Timer_Get_Seconds(void);
void     Timebase_Timer_Set_SubSeconds(uint32_t value);
void     Timebase_Timer_Set_Seconds(uint32_t value);
void     Timebase_Timer_Delay_SubSeconds(uint16_t value);
void     Timebase_Timer_Delay_Seconds(uint16_t value);

//UpCounter Functions
uint8_t  Timebase_UpCounter_Get_Status(uint8_t window);
void     Timebase_UpCounter_Set_Status(uint8_t window, uint8_t value);
int32_t  Timebase_UpCounter_Get_Value(uint8_t window);
void     Timebase_UpCounter_Set_Value(uint8_t window, int32_t value);
uint32_t Timebase_UpCounter_Get_EndValue(uint8_t window);
void     Timebase_UpCounter_Set_EndValue(uint8_t window, uint32_t value);
uint32_t Timebase_UpCounter_Get_TargetValue(uint8_t window);
void     Timebase_UpCounter_Set_TargetValue(uint8_t window, uint32_t value);
uint32_t Timebase_UpCounter_Get_TemporaryValue(uint8_t window);
void     Timebase_UpCounter_Set_TemporaryValue(uint8_t window, uint32_t value);
uint32_t Timebase_UpCounter_Get_PeriodValue(uint8_t window);
void     Timebase_UpCounter_Set_PeriodValue(uint8_t window, uint32_t value);
uint8_t  Timebase_UpCounter_Get_Period_Flag(uint8_t window);
void     Timebase_UpCounter_Set_Period_Flag(uint8_t window);
void     Timebase_UpCounter_Clear_Period_Flag(uint8_t window);
void     Timebase_UpCounter_Reset(uint8_t window);
void     Timebase_UpCounter_Clear_All_Flags(uint8_t window);
void     Timebase_UpCounter_Start(uint8_t window);
void     Timebase_UpCounter_Stop(uint8_t window);
void     Timebase_UpCounter_Set_Securely(uint8_t window, uint32_t value);
void     Timebase_UpCounter_Set_Forcefully(uint8_t window, uint32_t value);
void     Timebase_UpCounter_Update(uint8_t window);
uint8_t  Timebase_UpCounter_Expired(uint8_t window);
uint8_t  Timebase_UpCounter_Expired_Event(uint8_t window);
void     Timebase_UpCounter_Set_Period_Value_Securely(uint8_t window, uint32_t value);
uint32_t Timebase_UpCounter_Get_Remaining_Period_Value(uint8_t window);
uint8_t  Timebase_UpCounter_Period_Value_Expired(uint8_t window);
uint8_t  Timebase_UpCounter_Period_Value_Expired_Event(uint8_t window);
void     Timebase_UpCounter_Update_All(void);
void     Timebase_UpCounter_Reset_All(void);

//DownCounter Functions
uint8_t  Timebase_DownCounter_Get_Status(uint8_t window);
void     Timebase_DownCounter_Set_Status(uint8_t window, uint8_t value);
int32_t Timebase_DownCounter_Get_Value(uint8_t window);
void     Timebase_DownCounter_Set_Value(uint8_t window, int32_t value);
uint32_t Timebase_DownCounter_Get_EndValue(uint8_t window);
void     Timebase_DownCounter_Set_EndValue(uint8_t window, uint32_t value);
uint32_t Timebase_DownCounter_Get_PeriodValue(uint8_t window);
void     Timebase_DownCounter_Set_PeriodValue(uint8_t window, uint32_t value);
uint8_t  Timebase_DownCounter_Get_Period_Flag(uint8_t window);
void     Timebase_DownCounter_Set_Period_Flag(uint8_t window);
void     Timebase_DownCounter_Clear_Period_Flag(uint8_t window);
void     Timebase_DownCounter_Reset(uint8_t window);
void     Timebase_DownCounter_Clear_All_Flags(uint8_t window);
void     Timebase_DownCounter_Start(uint8_t window);
void     Timebase_DownCounter_Stop(uint8_t window);
void     Timebase_DownCounter_Set_Securely(uint8_t window, uint32_t value);
void     Timebase_DownCounter_Set_Forcefully(uint8_t window, uint32_t value);
void     Timebase_DownCounter_Update(uint8_t window);
uint8_t  Timebase_DownCounter_Expired(uint8_t window);
uint8_t  Timebase_DownCounter_Expired_Event(uint8_t window);
void     Timebase_DownCounter_Set_Period_Value_Securely(uint8_t window, uint32_t value);
uint32_t Timebase_DownCounter_Get_Remaining_Period_Value(uint8_t window);
uint8_t  Timebase_DownCounter_Period_Value_Expired(uint8_t window);
uint8_t  Timebase_DownCounter_Period_Value_Expired_Event(uint8_t window);
void     Timebase_DownCounter_Update_All(void);
void     Timebase_DownCounter_Reset_All(void);

//Common Functions
void     Timebase_Reset(void);
void     Timebase_Init(void);
void     Timebase_Main_Loop_Executables(void);
void     Timebase_ISR_Executables(void);

void     Timer_Callback_1(void* arg);
uint32_t ESP_Timer_Get_Time (void);
void     ESP_Timer_Start (void);
void     Reset_ESP_Timer(void);

