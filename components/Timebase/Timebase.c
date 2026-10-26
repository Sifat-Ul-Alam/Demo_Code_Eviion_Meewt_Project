#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "driver/gptimer.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "stdio.h"

#include "Timebase_2.h"
#include "Timebase.h"


#define TIMER_INTERVAL_SEC          1
#define bool _Bool

volatile uint64_t   Timer_Counter_1 = 0;
esp_timer_handle_t  Periodic_Timer_1;

typedef struct {
  uint64_t          Event_Count;
}
queue_element_t;

typedef struct { 
  gptimer_handle_t  Def_Time;
  gptimer_handle_t  Uart_Data;
  QueueHandle_t     Queue_Def_Time;
  QueueHandle_t     Queue_Uart_Data;
}
Timer_parameter;

queue_element_t  Ele;
Timer_parameter  Timer__Parameter;

bool IRAM_ATTR Gp_Timer_Alarm(gptimer_handle_t timer, const gptimer_alarm_event_data_t *edata, void *user_data){
    BaseType_t high_task_awoken = pdFALSE;
    QueueHandle_t queue = (QueueHandle_t)user_data;
    // stop timer immediately
    gptimer_stop(timer);
    // Retrieve count value and send to queue
    queue_element_t Ele = {
        .Event_Count = edata->count_value
    };
    xQueueSendFromISR(queue, &Ele, &high_task_awoken);
    // return whether we need to yield at the end of ISR
    return (high_task_awoken == pdTRUE);
}
void GpTimer_Init_Dependent(gptimer_handle_t *gptimer) {
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,    // 1MHz, 1 tick=1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, gptimer)); 
}
void GpTimer_Init_Independent(gptimer_handle_t *gptimer, uint64_t resolution){
    gptimer_config_t timer_config = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = resolution, // 1MHz, 1 tick=1us
    };
    ESP_ERROR_CHECK(gptimer_new_timer(&timer_config, gptimer));
    gptimer_enable(*gptimer);
    ESP_ERROR_CHECK(gptimer_start(*gptimer));
}
void GpTimer_Alarm_Set(gptimer_handle_t *gptimer, QueueHandle_t *queue, uint64_t alarm_time){
    GpTimer_Init_Dependent(gptimer);
    *queue = xQueueCreate(20, sizeof(queue_element_t));
    gptimer_event_callbacks_t cbs = {
        .on_alarm = Gp_Timer_Alarm,
    };
    gptimer_register_event_callbacks(*gptimer, &cbs, *queue);
    gptimer_enable(*gptimer);
    gptimer_alarm_config_t alarm_config1 = {
        .alarm_count = alarm_time,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true, // enable auto-reload
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(*gptimer, &alarm_config1));
    ESP_ERROR_CHECK(gptimer_start(*gptimer));
}
void GpTimer_Alarm_Reset_Auto_Reload(gptimer_handle_t *gptimer,uint64_t alarm_time){
    gptimer_alarm_config_t alarm_config1 = {
        .alarm_count = alarm_time,
        .reload_count = 0,
        .flags.auto_reload_on_alarm = true, // enable auto-reload
    };
    ESP_ERROR_CHECK(gptimer_set_alarm_action(*gptimer, &alarm_config1));
    ESP_ERROR_CHECK(gptimer_start(*gptimer));
}
void GpTimer_Alarm_Reset_Run_Time(gptimer_handle_t gptimer,uint64_t value) {
    gptimer_set_raw_count(gptimer,value);
}
uint64_t Get_GpTimer_Run_Time(gptimer_handle_t gptimer){
   uint64_t  count;
   gptimer_get_raw_count(gptimer, &count);
   return (count);
}
////////////////////////////////////@ESP-32 Hardware GP Timers//////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
void Def_Timer_Start(void){
   GpTimer_Alarm_Set (&Timer__Parameter.Def_Time, &Timer__Parameter.Queue_Def_Time, 1000);
}
void Def_Timer_Reset(void){
   GpTimer_Alarm_Reset_Auto_Reload (&Timer__Parameter.Def_Time, 1000);
}
uint8_t Get_Def_Timer_Complete_Flag (void){
  if(xQueueReceive(Timer__Parameter.Queue_Def_Time, &Ele, pdMS_TO_TICKS(0))){
    return 1;
  }else{
    return 0;
  }
}
uint64_t Get_Def_Timer_Run_Time (void){
   return Get_GpTimer_Run_Time(Timer__Parameter.Def_Time);
}
////////////////////////////////////////////////////////////////////////////////////////////////
void ComPort_UartData_Update_Timer_Start(void){
  GpTimer_Alarm_Set (&Timer__Parameter.Uart_Data, &Timer__Parameter.Queue_Uart_Data, 500000);
}
void ComPort_UartData_Update_Timer_Reset(void){
  GpTimer_Alarm_Reset_Auto_Reload (&Timer__Parameter.Uart_Data, 500000);
}
uint8_t Get_ComPort_UartData_Timer_Complete_Flag (void){
  if(xQueueReceive(Timer__Parameter.Queue_Uart_Data, &Ele, pdMS_TO_TICKS(0))){
    return 1;
  }else{
    return 0;
  }
}
////////////////////////////////////@ESP-32 Hardware GP Timers//////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
void IRAM_ATTR Timer_Callback_1(void* arg){
    Timer_Counter_1++;
    Timebase_ISR_Executables();
}
uint32_t ESP_Timer_Get_Time (void){
   return Timer_Counter_1;
}
void ESP_Timer_Start (void){
    // Configure the Timer
    esp_timer_create_args_t Timer_Args_1 = {
        .callback = &Timer_Callback_1,
        .name = "Periodic_Timer_1"
    };
    esp_timer_create(&Timer_Args_1, &Periodic_Timer_1);
    // Start the Timer to fire every 2 seconds
    esp_timer_start_periodic(Periodic_Timer_1, TIMER_INTERVAL_SEC * 1000000);
}
void Reset_ESP_Timer(void) {
    // Stop the Timer
    esp_timer_stop(Periodic_Timer_1);
    // Reset the counter
    Timer_Counter_1 = 0;
    // Start the Timer again
    esp_timer_start_periodic(Periodic_Timer_1, TIMER_INTERVAL_SEC * 1000000);
}

////////////////////////////////////////  Device Specific Functions  ///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//#define  TIMEBASE_UPCOUNTER        
#define  TIMEBASE_DOWNCOUNTER                10
//#define  TIMEBASE_TOKEN_FUNCTIONS            

typedef union {
  struct {
    volatile uint8_t          WatchDogTimer  :1;
    volatile uint8_t          RealTimeCounter:1;
    volatile uint8_t          GeneralTimer   :1;
    volatile uint8_t          TimerIndex     :5;
  };
  volatile uint8_t            Value            ;
} timebase_timer_t;

typedef struct timebase_time_t{
  volatile uint32_t           LastSample       ;
  volatile uint32_t           SubSeconds       ;
  volatile uint32_t           Seconds          ;
}timebase_time_t;

typedef union {
  struct {
    uint8_t                   PeriodFlag     :1;
    uint8_t                   Value          :4;
  };
  volatile uint8_t            StatusByte       ;
} timebase_status_t;


#ifdef TIMEBASE_UPCOUNTER
typedef struct timebase_upcounter_t{
  timebase_status_t           Status           ;
  int32_t                     EndValue         ;
  int32_t                     Target           ;
  int32_t                     Temporary        ;
  int32_t                     Value            ;
  int32_t                     PeriodValue      ;
}timebase_upcounter_t;
#endif

#ifdef TIMEBASE_DOWNCOUNTER
typedef struct timebase_downcounter_t{
  timebase_status_t           Status           ;
  int32_t                     EndValue         ;
  int32_t                     Value            ;
  int32_t                     PeriodValue      ;
}timebase_downcounter_t;
#endif

typedef struct timebase_config_t{
  timebase_timer_t            TimerType        ;
  volatile uint16_t           UpdateRate       ;
}timebase_config_t;

typedef struct timebase_t{
  timebase_config_t      Config                             ;
  timebase_time_t        Time                               ;
  
  #ifdef TIMEBASE_TOKEN_FUNCTIONS
  volatile uint8_t       ActiveTokens                       ;
  #endif
  
  #ifdef TIMEBASE_UPCOUNTER
  timebase_upcounter_t   UpCounter[TIMEBASE_UPCOUNTER]      ;
  #endif
  
  #ifdef TIMEBASE_DOWNCOUNTER
  timebase_downcounter_t DownCounter[TIMEBASE_DOWNCOUNTER]  ;
  #endif
  volatile uint8_t       FunctionUpdatesRequired            ;
  
}timebase_t;

timebase_t Timebase_type;
timebase_t *Timebase;

void Timebase_Struct_Init(void){
  Timebase=&Timebase_type;
  Timebase->Config.TimerType.Value = 0;
  Timebase->Config.TimerType.GeneralTimer = 1;
  Timebase->Config.TimerType.TimerIndex = 0;
  Timebase->Config.UpdateRate = 1;
  Timebase->Time.SubSeconds = 0;
  Timebase->Time.Seconds = 0;
  Timebase->Time.LastSample = 0;
  
  #ifdef TIMEBASE_TOKEN_FUNCTIONS
  Timebase->ActiveTokens = 0;
  #endif

  #ifdef TIMEBASE_UPCOUNTER
  for(uint8_t i=0; i < TIMEBASE_UPCOUNTER; i++){
    Timebase->UpCounter[i].Status.StatusByte = 0;  
    Timebase->UpCounter[i].EndValue = 0;
    Timebase->UpCounter[i].Target = 0;
    Timebase->UpCounter[i].Temporary = 0;    
    Timebase->UpCounter[i].Value = 0;
    Timebase->UpCounter[i].PeriodValue = 0;
  }
  #endif

  #ifdef TIMEBASE_DOWNCOUNTER
  for(uint8_t i=0; i < TIMEBASE_DOWNCOUNTER; i++){
    Timebase->DownCounter[i].Status.StatusByte = 0; 
    Timebase->DownCounter[i].EndValue = 0;
    Timebase->DownCounter[i].Value = 0;
    Timebase->DownCounter[i].PeriodValue = 0;
  }
  #endif
  Timebase->FunctionUpdatesRequired=0;
}

//Token Functions
#ifdef TIMEBASE_TOKEN_FUNCTIONS
uint8_t Timebase_Token_Executing(void){
  return Timebase->ActiveTokens;
}

void Timebase_Token_Add(void){ 
  if( Timebase_Token_Executing() < 63){
    Timebase->ActiveTokens+=1;
  }
}

void Timebase_Token_Remove(void){
  if( Timebase_Token_Executing() > 0){
    Timebase->ActiveTokens-=1;
  }
}

void Timebase_Token_Remove_All(void){
  Timebase->ActiveTokens=0;
}
#endif


//Timer Functions
uint32_t Timebase_Timer_Get_SubSeconds(void){
  return Timebase->Time.SubSeconds;
}

uint32_t Timebase_Timer_Get_Seconds(void){
  return Timebase->Time.Seconds;
}

void Timebase_Timer_Set_SubSeconds(uint32_t value){
  Timebase->Time.SubSeconds=value;
}

void Timebase_Timer_Set_Seconds(uint32_t value){
  Timebase->Time.Seconds=value;
}

void Timebase_Timer_Delay_SubSeconds(uint16_t value){
  #ifdef TIMEBASE_TOKEN_FUNCTIONS
  Timebase_Token_Add();
  #endif
  
  uint32_t temp = Timebase_Timer_Get_SubSeconds() + value;
  while(temp > Timebase_Timer_Get_SubSeconds());
  
  #ifdef TIMEBASE_TOKEN_FUNCTIONS
  Timebase_Token_Remove();
  #endif
}

void Timebase_Timer_Delay_Seconds(uint16_t value){
  uint32_t temp = Timebase_Timer_Get_Seconds() + value;
  while(temp > Timebase_Timer_Get_SubSeconds());
}


//UpCounter Functions
#ifdef TIMEBASE_UPCOUNTER
uint8_t Timebase_UpCounter_Get_Status(uint8_t window){
  return Timebase->UpCounter[window].Status.Value;
}

void Timebase_UpCounter_Set_Status(uint8_t window, uint8_t value){
  Timebase->UpCounter[window].Status.Value = value;
}

uint32_t Timebase_UpCounter_Get_Value(uint8_t window){
  return (uint32_t)Timebase->UpCounter[window].Value;
}

void Timebase_UpCounter_Set_Value(uint8_t window, int32_t value){
  Timebase->UpCounter[window].Value = value;
}

uint32_t Timebase_UpCounter_Get_EndValue(uint8_t window){
  return (uint32_t)Timebase->UpCounter[window].EndValue;
}

void Timebase_UpCounter_Set_EndValue(uint8_t window, uint32_t value){
  Timebase->UpCounter[window].EndValue = value;
}

uint32_t Timebase_UpCounter_Get_TargetValue(uint8_t window){
  return (uint32_t)Timebase->UpCounter[window].Target;
}

void Timebase_UpCounter_Set_TargetValue(uint8_t window, uint32_t value){
  Timebase->UpCounter[window].Target = value;
}

uint32_t Timebase_UpCounter_Get_TemporaryValue(uint8_t window){
  return (uint32_t)Timebase->UpCounter[window].Temporary;
}

void Timebase_UpCounter_Set_TemporaryValue(uint8_t window, uint32_t value){
  Timebase->UpCounter[window].Temporary = value;
}

uint32_t Timebase_UpCounter_Get_PeriodValue(uint8_t window){
  return (uint32_t)Timebase->UpCounter[window].PeriodValue;
}

void Timebase_UpCounter_Set_PeriodValue(uint8_t window, uint32_t value){
  Timebase->UpCounter[window].PeriodValue = value;
}

uint8_t Timebase_UpCounter_Get_Period_Flag(uint8_t window){
  return Timebase->UpCounter[window].Status.PeriodFlag;
}

void Timebase_UpCounter_Set_Period_Flag(uint8_t window){
  Timebase->UpCounter[window].Status.PeriodFlag = 1;
}

void Timebase_UpCounter_Clear_Period_Flag(uint8_t window){
  Timebase->UpCounter[window].Status.PeriodFlag = 0;
}

void Timebase_UpCounter_Reset(uint8_t window){
  Timebase_UpCounter_Set_Status(window, 0);
  Timebase_UpCounter_Set_Value(window, 0);
  Timebase_UpCounter_Set_EndValue(window, 0);
  Timebase_UpCounter_Set_TargetValue(window, 0);
  Timebase_UpCounter_Set_TemporaryValue(window, 0);
  Timebase_UpCounter_Set_PeriodValue(window, 0);
  Timebase_UpCounter_Clear_Period_Flag(window);
} 

void Timebase_UpCounter_Clear_All_Flags(uint8_t window){
  Timebase_UpCounter_Reset( window );
}

void Timebase_UpCounter_Start(uint8_t window){
  if(Timebase_UpCounter_Get_Status(window) != 1){
    Timebase_UpCounter_Set_Status(window , 1);    
  }
}

void Timebase_UpCounter_Stop(uint8_t window){
  if(Timebase_UpCounter_Get_Status(window) != 2){
    Timebase_UpCounter_Set_Status(window , 2);
  }
}


void Timebase_UpCounter_Set_Securely(uint8_t window, uint32_t value){
  if( Timebase_UpCounter_Get_Status( window ) == 0 ){
    Timebase_UpCounter_Set_Value(window, 0);
    Timebase_UpCounter_Set_TemporaryValue(window, 0);
    Timebase_UpCounter_Set_TargetValue(window, value);
    Timebase_UpCounter_Set_EndValue(window, Timebase_Timer_Get_Seconds() + value);  
    Timebase_UpCounter_Start(window);
  }
}

void Timebase_UpCounter_Set_Forcefully(uint8_t window, uint32_t value){
  Timebase_UpCounter_Reset( window );
  Timebase_UpCounter_Set_Securely( window, value );
} 

void Timebase_UpCounter_Update(uint8_t window){
  if( Timebase_UpCounter_Get_Status( window ) == 1 ){ 
    Timebase_UpCounter_Set_TemporaryValue(window, Timebase_UpCounter_Get_EndValue(window) - Timebase_Timer_Get_Seconds() );
    Timebase_UpCounter_Set_Value(window, Timebase_UpCounter_Get_TargetValue(window) - Timebase_UpCounter_Get_TemporaryValue(window) );
    if(Timebase_UpCounter_Get_TemporaryValue(window) <= 0){
      Timebase_UpCounter_Set_EndValue(window, 0);
      Timebase_UpCounter_Set_TemporaryValue(window, 0);
      Timebase_UpCounter_Set_Value(window, Timebase_UpCounter_Get_TargetValue(window));
      Timebase_UpCounter_Set_Status(window, 4);
    }
  } else if (Timebase_UpCounter_Get_Status( window ) == 2){
    Timebase_UpCounter_Set_EndValue(window, Timebase_UpCounter_Get_TemporaryValue(window) + Timebase_Timer_Get_Seconds());
    Timebase_UpCounter_Set_Value(window, Timebase_UpCounter_Get_TargetValue(window) - Timebase_UpCounter_Get_TemporaryValue(window));
  }
}


uint8_t Timebase_UpCounter_Expired(uint8_t window){
  if(Timebase_UpCounter_Get_Status( window ) == 4){
    return 1;
  }else{
    return 0;
  }
}

uint8_t Timebase_UpCounter_Expired_Event(uint8_t window){
  if(Timebase_UpCounter_Get_Status( window ) == 4){
    Timebase_UpCounter_Clear_All_Flags( window );
    return 1;
  }else{
    return 0;
  }
}

void Timebase_UpCounter_Set_Period_Value_Securely(uint8_t window, uint32_t value){
  if(Timebase_UpCounter_Get_Period_Flag( window ) == 0){
    Timebase_UpCounter_Set_PeriodValue( window, Timebase_UpCounter_Get_Value( window ) + value);
    Timebase_UpCounter_Set_Period_Flag( window);
  }
}

uint32_t Timebase_UpCounter_Get_Remaining_Period_Value(uint8_t window){
  int32_t temp = Timebase_UpCounter_Get_PeriodValue( window ) - Timebase_UpCounter_Get_Value( window );
  if(temp < 0){
    temp = 0;
  }
  return temp;
}

uint8_t Timebase_UpCounter_Period_Value_Expired(uint8_t window){
  if( (Timebase_UpCounter_Get_Remaining_Period_Value( window ) == 0) && Timebase_UpCounter_Get_Period_Flag( window )){
    return 1;
  }else{
    return 0;
  }
}

uint8_t Timebase_UpCounter_Period_Value_Expired_Event(uint8_t window){
  if(Timebase_UpCounter_Period_Value_Expired( window ) == 1){
    Timebase_UpCounter_Clear_Period_Flag( window );
    return 1;
  }else{
    return 0;
  }
}

void Timebase_UpCounter_Update_All(void){
  for(uint8_t i=0; i<TIMEBASE_UPCOUNTER; i++){
    Timebase_UpCounter_Update(i);
  }
}

void Timebase_UpCounter_Reset_All(void){
  for(uint8_t i=0; i<TIMEBASE_UPCOUNTER; i++){
    Timebase_UpCounter_Reset(i);
  }
}
#endif



#ifdef TIMEBASE_DOWNCOUNTER
uint8_t Timebase_DownCounter_Get_Status(uint8_t window){
  return Timebase->DownCounter[window].Status.Value;
}

void Timebase_DownCounter_Set_Status(uint8_t window, uint8_t value){
  Timebase->DownCounter[window].Status.Value = value;
}

int32_t Timebase_DownCounter_Get_Value(uint8_t window){
  return (int32_t)Timebase->DownCounter[window].Value;
}

void Timebase_DownCounter_Set_Value(uint8_t window, int32_t value){
  Timebase->DownCounter[window].Value = value;
}

uint32_t Timebase_DownCounter_Get_EndValue(uint8_t window){
  return (uint32_t)Timebase->DownCounter[window].EndValue;
}

void Timebase_DownCounter_Set_EndValue(uint8_t window, uint32_t value){
  Timebase->DownCounter[window].EndValue = value;
}


uint32_t Timebase_DownCounter_Get_PeriodValue(uint8_t window){
  return (uint32_t)Timebase->DownCounter[window].PeriodValue;
}

void Timebase_DownCounter_Set_PeriodValue(uint8_t window, uint32_t value){
  // if(value < 0){
  //   Timebase->DownCounter[window].PeriodValue = 0;
  // }else{
  //   Timebase->DownCounter[window].PeriodValue = value;
  // }

  Timebase->DownCounter[window].PeriodValue = value;
}

uint8_t Timebase_DownCounter_Get_Period_Flag(uint8_t window){
  return (uint8_t)Timebase->DownCounter[window].Status.PeriodFlag;
}

void Timebase_DownCounter_Set_Period_Flag(uint8_t window){
  Timebase->DownCounter[window].Status.PeriodFlag = 1;
}

void Timebase_DownCounter_Clear_Period_Flag(uint8_t window){
  Timebase->DownCounter[window].Status.PeriodFlag = 0;
}


void Timebase_DownCounter_Reset(uint8_t window){
  Timebase_DownCounter_Set_EndValue(window, 0);
  Timebase_DownCounter_Set_Value(window, 0);
  Timebase_DownCounter_Set_Status(window, 0);
  Timebase_DownCounter_Clear_Period_Flag(window);
} 

void Timebase_DownCounter_Clear_All_Flags(uint8_t window){
  Timebase_DownCounter_Reset( window );
}

void Timebase_DownCounter_Start(uint8_t window){
  if(Timebase_DownCounter_Get_Status(window) != 1){
    Timebase_DownCounter_Set_Status(window, 1); 
  }
}

void Timebase_DownCounter_Stop(uint8_t window){
  if(Timebase_DownCounter_Get_Status(window) != 2){
    Timebase_DownCounter_Set_Status(window, 2);    
  }
}


void Timebase_DownCounter_Set_Securely(uint8_t window, uint32_t value){
  if( Timebase_DownCounter_Get_Status( window ) == 0 ){
    Timebase_DownCounter_Set_Value(window, value);
    Timebase_DownCounter_Set_EndValue(window, Timebase_Timer_Get_Seconds() + value);
    Timebase_DownCounter_Start(window);
  }
}

void Timebase_DownCounter_Set_Forcefully(uint8_t window, uint32_t value){
  Timebase_DownCounter_Reset( window );
  Timebase_DownCounter_Set_Securely( window, value );
} 

void Timebase_DownCounter_Update(uint8_t window){
  if( Timebase_DownCounter_Get_Status( window ) == 1 ){ 
    int32_t temp = Timebase_DownCounter_Get_EndValue(window);
    temp-=Timebase_Timer_Get_Seconds();
    Timebase_DownCounter_Set_Value(window, temp);
    if(Timebase_DownCounter_Get_Value(window) <= 0){
      Timebase_DownCounter_Reset(window);
      Timebase_DownCounter_Set_Status(window, 4);
    }
  } else if (Timebase_DownCounter_Get_Status( window ) == 2){ 
    Timebase_DownCounter_Set_EndValue(window, Timebase_DownCounter_Get_Value(window) + Timebase_Timer_Get_Seconds());
  }
}

uint8_t Timebase_DownCounter_Expired(uint8_t window){
  if(Timebase_DownCounter_Get_Status( window ) == 4){
    return 1;
  }else{
    return 0;
  }
}

uint8_t Timebase_DownCounter_Expired_Event(uint8_t window){
  if(Timebase_DownCounter_Get_Status( window ) == 4){
    Timebase_DownCounter_Clear_All_Flags( window );
    return 1;
  }else{
    return 0;
  }
}


void Timebase_DownCounter_Set_Period_Value_Securely(uint8_t window, uint32_t value){
  if(Timebase_DownCounter_Get_Period_Flag( window ) == 0){
    Timebase_DownCounter_Set_PeriodValue(window, Timebase_DownCounter_Get_Value(window) - value);
    Timebase_DownCounter_Set_Period_Flag(window);
  }
}

uint32_t Timebase_DownCounter_Get_Remaining_Period_Value(uint8_t window){
  int32_t temp = Timebase_DownCounter_Get_Value(window) - Timebase_DownCounter_Get_PeriodValue(window) ;
  if(temp < 0){
    temp = 0;
  }
  return temp;
}

uint8_t Timebase_DownCounter_Period_Value_Expired(uint8_t window){
  if( (Timebase_DownCounter_Get_Remaining_Period_Value( window ) == 0) && Timebase_DownCounter_Get_Period_Flag( window )){
    return 1;
  }else{
    return 0;
  }
}

uint8_t Timebase_DownCounter_Period_Value_Expired_Event(uint8_t window){
  if(Timebase_DownCounter_Period_Value_Expired( window ) == 1){
    Timebase_DownCounter_Clear_Period_Flag( window );
    return 1;
  }else{
    return 0;
  }
}


void Timebase_DownCounter_Update_All(void){
  for(uint8_t i=0; i<TIMEBASE_DOWNCOUNTER; i++){
    Timebase_DownCounter_Update(i);
  }
}

void Timebase_DownCounter_Reset_All(void){
  for(uint8_t i=0; i<TIMEBASE_DOWNCOUNTER; i++){
    Timebase_DownCounter_Reset(i);
  }
}
#endif

void Timebase_Reset(void){
  #ifdef TIMEBASE_UPCOUNTER
  Timebase_UpCounter_Reset_All();
  #endif
  #ifdef TIMEBASE_DOWNCOUNTER
  Timebase_DownCounter_Reset_All();
  #endif
}

void Timebase_Init(void){
  Timebase_Struct_Init();
  ESP_Timer_Start();
  Timebase_Reset();
}

void Timebase_Main_Loop_Executables(void){
  if(Timebase->FunctionUpdatesRequired){
    #ifdef TIMEBASE_UPCOUNTER
    Timebase_UpCounter_Update_All();
    #endif
    #ifdef TIMEBASE_DOWNCOUNTER
    Timebase_DownCounter_Update_All();
    #endif
	Timebase->FunctionUpdatesRequired=0;
  }
}

void Timebase_ISR_Executables(void){
  Timebase->Time.SubSeconds++;
  if((Timebase->Time.SubSeconds % Timebase->Config.UpdateRate) == 0){
    Timebase->Time.Seconds++;
	Timebase->FunctionUpdatesRequired=1;
	#ifdef TIMEBASE_TOKEN_FUNCTIONS
    if(Timebase_Token_Executing() == 0){
      Timebase->Time.SubSeconds = 0;
    }
	#endif
  }
}
