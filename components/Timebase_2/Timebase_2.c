#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
//#include "driver/gptimer.h"
#include "freertos/task.h"
//#include "driver/gpio.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "stdio.h"


#define TIMER_INTERVAL_MILI_SEC      1

volatile uint64_t  Timer_Counter_2 = 0;
esp_timer_handle_t Periodic_Timer_2;

////////////////////////////////////////  Device Specific Functions  /////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define  TIMEBASE_DOWNCOUNTER_2               30
//#define  TIMEBASE_TOKEN_FUNCTIONS_2           5

typedef union {
  struct {
    volatile uint8_t          WatchDogTimer  :1;
    volatile uint8_t          RealTimeCounter:1;
    volatile uint8_t          GeneralTimer   :1;
    volatile uint8_t          TimerIndex     :5;
  };
  volatile uint8_t            Value            ;
} timebase_timer_t2;

typedef struct timebase_time_t2{
  volatile uint32_t           LastSample       ;
  volatile uint32_t           SubSeconds       ;
  volatile uint32_t           Seconds          ;
}timebase_time_t2;

typedef union {
  struct {
    uint8_t                   PeriodFlag     :1;
    uint8_t                   Value          :4;
  };
  volatile uint8_t            StatusByte       ;
} timebase_status_t2;

#ifdef TIMEBASE_DOWNCOUNTER_2
typedef struct timebase_downcounter_t2{
  timebase_status_t2          Status           ;
  int32_t                     EndValue         ;
  int32_t                     Value            ;
  int32_t                     PeriodValue      ;
}timebase_downcounter_t2;
#endif

typedef struct timebase_config_t2{
  timebase_timer_t2          TimerType         ;
  volatile uint16_t          UpdateRate        ;
}timebase_config_t2;

typedef struct timebase_t2{
  timebase_config_t2      Config              ;
  timebase_time_t2        Time                ;
  
  #ifdef TIMEBASE_TOKEN_FUNCTIONS_2
  volatile uint8_t       ActiveTokens                       ;
  #endif
  
  #ifdef TIMEBASE_DOWNCOUNTER_2
  timebase_downcounter_t2 DownCounter[TIMEBASE_DOWNCOUNTER_2]   ;
  #endif
  volatile uint8_t       FunctionUpdatesRequired            ;
  
}timebase_t2;

timebase_t2 Timebase_type2;
timebase_t2 *Timebase2;

void Timebase_Struct_Init_2(void){
  Timebase2=&Timebase_type2;
  Timebase2->Config.TimerType.Value = 0;
  Timebase2->Config.TimerType.GeneralTimer = 1;
  Timebase2->Config.TimerType.TimerIndex = 0;
  Timebase2->Config.UpdateRate = 1;
  Timebase2->Time.SubSeconds = 0;
  Timebase2->Time.Seconds = 0;
  Timebase2->Time.LastSample = 0;
  
  #ifdef TIMEBASE_TOKEN_FUNCTIONS_2
  Timebase->ActiveTokens = 0;
  #endif

  #ifdef TIMEBASE_DOWNCOUNTER_2
  for(uint8_t i=0; i < TIMEBASE_DOWNCOUNTER_2; i++){
    Timebase2->DownCounter[i].Status.StatusByte = 0; 
    Timebase2->DownCounter[i].EndValue = 0;
    Timebase2->DownCounter[i].Value = 0;
    Timebase2->DownCounter[i].PeriodValue = 0;
  }
  #endif
  Timebase2->FunctionUpdatesRequired=0;
}

//Token Functions
#ifdef TIMEBASE_TOKEN_FUNCTIONS_2
uint8_t Timebase_Token_Executing_2(void){
  return Timebase2->ActiveTokens;
}

void Timebase_Token_Add_2(void){ 
  if( Timebase_Token_Executing_2() < 63){
    Timebase2->ActiveTokens+=1;
  }
}

void Timebase_Token_Remove_2(void){
  if( Timebase_Token_Executing_2() > 0){
    Timebase2->ActiveTokens-=1;
  }
}

void Timebase_Token_Remove_All_2(void){
  Timebase2->ActiveTokens=0;
}
#endif

//Timer Functions
uint32_t Timebase_Timer_Get_SubSeconds_2(void){
  return Timebase2->Time.SubSeconds;
}

uint32_t Timebase_Timer_Get_Seconds_2(void){
  return Timebase2->Time.Seconds;
}

void Timebase_Timer_Set_SubSeconds_2(uint32_t value){
  Timebase2->Time.SubSeconds=value;
}

void Timebase_Timer_Set_Seconds_2(uint32_t value){
  Timebase2->Time.Seconds=value;
}

void Timebase_Timer_Delay_SubSeconds_2(uint16_t value){
  #ifdef TIMEBASE_TOKEN_FUNCTIONS_2
  Timebase_Token_Add_2();
  #endif
  
  uint32_t temp = Timebase_Timer_Get_SubSeconds_2() + value;
  while(temp > Timebase_Timer_Get_SubSeconds_2());
  
  #ifdef TIMEBASE_TOKEN_FUNCTIONS_2
  Timebase_Token_Remove_2();
  #endif
}

void Timebase_Timer_Delay_Seconds_2(uint16_t value){
  uint32_t temp = Timebase_Timer_Get_Seconds_2() + value;
  while(temp > Timebase_Timer_Get_SubSeconds_2());
}

#ifdef TIMEBASE_DOWNCOUNTER_2
uint8_t Timebase_DownCounter_Get_Status_2(uint8_t window){
  return Timebase2->DownCounter[window].Status.Value;
}

void Timebase_DownCounter_Set_Status_2(uint8_t window, uint8_t value){
  Timebase2->DownCounter[window].Status.Value = value;
}

int32_t Timebase_DownCounter_Get_Value_2(uint8_t window){
  return (int32_t)Timebase2->DownCounter[window].Value;
}

void Timebase_DownCounter_Set_Value_2(uint8_t window, int32_t value){
  Timebase2->DownCounter[window].Value = value;
}

uint32_t Timebase_DownCounter_Get_EndValue_2(uint8_t window){
  return (uint32_t)Timebase2->DownCounter[window].EndValue;
}

void Timebase_DownCounter_Set_EndValue_2(uint8_t window, uint32_t value){
  Timebase2->DownCounter[window].EndValue = value;
}


uint32_t Timebase_DownCounter_Get_PeriodValue_2(uint8_t window){
  return (uint32_t)Timebase2->DownCounter[window].PeriodValue;
}

void Timebase_DownCounter_Set_PeriodValue_2(uint8_t window, int32_t value){
  if(value < 0){
    Timebase2->DownCounter[window].PeriodValue = 0;
  }else{
    Timebase2->DownCounter[window].PeriodValue = value;
  }
}

uint8_t Timebase_DownCounter_Get_Period_Flag_2(uint8_t window){
  return (uint8_t)Timebase2->DownCounter[window].Status.PeriodFlag;
}

void Timebase_DownCounter_Set_Period_Flag_2(uint8_t window){
  Timebase2->DownCounter[window].Status.PeriodFlag = 1;
}

void Timebase_DownCounter_Clear_Period_Flag_2(uint8_t window){
  Timebase2->DownCounter[window].Status.PeriodFlag = 0;
}


void Timebase_DownCounter_Reset_2(uint8_t window){
  Timebase_DownCounter_Set_EndValue_2(window, 0);
  Timebase_DownCounter_Set_Value_2(window, 0);
  Timebase_DownCounter_Set_Status_2(window, 0);
  Timebase_DownCounter_Clear_Period_Flag_2(window);
} 

void Timebase_DownCounter_Clear_All_Flags_2(uint8_t window){
  Timebase_DownCounter_Reset_2( window );
}

void Timebase_DownCounter_Start_2(uint8_t window){
  if(Timebase_DownCounter_Get_Status_2(window) != 1){
    Timebase_DownCounter_Set_Status_2(window, 1); 
  }
}

void Timebase_DownCounter_Stop_2(uint8_t window){
  if(Timebase_DownCounter_Get_Status_2(window) != 2){
    Timebase_DownCounter_Set_Status_2(window, 2);    
  }
}


void Timebase_DownCounter_Set_Securely_2(uint8_t window, uint32_t value){
  if( Timebase_DownCounter_Get_Status_2( window ) == 0 ){
    Timebase_DownCounter_Set_Value_2(window, value);
    Timebase_DownCounter_Set_EndValue_2(window, Timebase_Timer_Get_Seconds_2() + value);
    Timebase_DownCounter_Start_2(window);
  }
}

void Timebase_DownCounter_Set_Forcefully_2(uint8_t window, uint32_t value){
  Timebase_DownCounter_Reset_2( window );
  Timebase_DownCounter_Set_Securely_2( window, value );
} 

void Timebase_DownCounter_Update_2(uint8_t window){
  if( Timebase_DownCounter_Get_Status_2( window ) == 1 ){ 
    int32_t temp = Timebase_DownCounter_Get_EndValue_2(window);
    temp-=Timebase_Timer_Get_Seconds_2();
    Timebase_DownCounter_Set_Value_2(window, temp);
    if(Timebase_DownCounter_Get_Value_2(window) <= 0){
      Timebase_DownCounter_Reset_2(window);
      Timebase_DownCounter_Set_Status_2(window, 4);
    }
  } else if (Timebase_DownCounter_Get_Status_2( window ) == 2){ 
    Timebase_DownCounter_Set_EndValue_2(window, Timebase_DownCounter_Get_Value_2(window) + Timebase_Timer_Get_Seconds_2());
  }
}

uint8_t Timebase_DownCounter_Expired_2(uint8_t window){
  if(Timebase_DownCounter_Get_Status_2( window ) == 4){
    return 1;
  }else{
    return 0;
  }
}

uint8_t Timebase_DownCounter_Expired_Event_2(uint8_t window){
  if(Timebase_DownCounter_Get_Status_2( window ) == 4){
    Timebase_DownCounter_Clear_All_Flags_2( window );
    return 1;
  }else{
    return 0;
  }
}


void Timebase_DownCounter_Set_Period_Value_Securely_2(uint8_t window, uint32_t value){
  if(Timebase_DownCounter_Get_Period_Flag_2( window ) == 0){
    Timebase_DownCounter_Set_PeriodValue_2(window, Timebase_DownCounter_Get_Value_2(window) - value);
    Timebase_DownCounter_Set_Period_Flag_2(window);
  }
}

uint32_t Timebase_DownCounter_Get_Remaining_Period_Value_2(uint8_t window){
  int32_t temp = Timebase_DownCounter_Get_Value_2(window) - Timebase_DownCounter_Get_PeriodValue_2(window) ;
  if(temp < 0){
    temp = 0;
  }
  return temp;
}

uint8_t Timebase_DownCounter_Period_Value_Expired_2(uint8_t window){
  if( (Timebase_DownCounter_Get_Remaining_Period_Value_2( window ) == 0) && Timebase_DownCounter_Get_Period_Flag_2( window )){
    return 1;
  }else{
    return 0;
  }
}

uint8_t Timebase_DownCounter_Period_Value_Expired_Event_2(uint8_t window){
  if(Timebase_DownCounter_Period_Value_Expired_2( window ) == 1){
    Timebase_DownCounter_Clear_Period_Flag_2( window );
    return 1;
  }else{
    return 0;
  }
}


void Timebase_DownCounter_Update_All_2(void){
  for(uint8_t i=0; i<TIMEBASE_DOWNCOUNTER_2; i++){
    Timebase_DownCounter_Update_2(i);
  }
}

void Timebase_DownCounter_Reset_All_2(void){
  for(uint8_t i=0; i<TIMEBASE_DOWNCOUNTER_2; i++){
    Timebase_DownCounter_Reset_2(i);
  }
}
#endif

void Timebase_Reset_2(void){
  #ifdef TIMEBASE_DOWNCOUNTER_2
  Timebase_DownCounter_Reset_All_2();
  #endif
}

void Timebase_Main_Loop_Executables_2(void){
  if(Timebase2->FunctionUpdatesRequired){
    #ifdef TIMEBASE_DOWNCOUNTER_2
    Timebase_DownCounter_Update_All_2();
    #endif
	Timebase2->FunctionUpdatesRequired=0;
  }
}

void Timebase_ISR_Executables_2(void){
    Timebase2->Time.Seconds++;
    Timebase2->FunctionUpdatesRequired=1;
}

void IRAM_ATTR Timer_Callback_2(void* arg){
    Timer_Counter_2++;
    Timebase_ISR_Executables_2();
}
uint64_t ESP_Timer_2_Get_Time (void){
   return Timer_Counter_2;
}
void ESP_Timer_2_Start (void){
    // Configure the Timer
    esp_timer_create_args_t Timer_Args_2 = {
        .callback = &Timer_Callback_2,
        .name = "Periodic_Timer_2"
    };
    esp_timer_create(&Timer_Args_2, &Periodic_Timer_2);
    // Start the Timer to fire every 1 miliseconds
    esp_timer_start_periodic(Periodic_Timer_2, TIMER_INTERVAL_MILI_SEC * 1000);
}
void Reset_ESP_Timer_2(void) {
    // Stop the Timer
    esp_timer_stop(Periodic_Timer_2);
    // Reset the counter
    Timer_Counter_2 = 0;
    // Start the Timer again
    esp_timer_start_periodic(Periodic_Timer_2, TIMER_INTERVAL_MILI_SEC * 1000);
}


void Timebase_Init_2(void){
  Timebase_Struct_Init_2();
  ESP_Timer_2_Start();
  Timebase_Reset_2();
}


