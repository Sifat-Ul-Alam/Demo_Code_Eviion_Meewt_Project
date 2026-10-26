#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"

#include "Power_Button.h"
#include "Display_Config.h"

#define TOTAL_BUTTON_NUMBER           1

#define BUTTON_SHORT_PRESS_TIME_MIN   5
#define BUTTON_SHORT_PRESS_TIME_MAX   1000
#define BUTTON_LONG_PRESS_TIME        1500

uint8_t    Button_Pins[TOTAL_BUTTON_NUMBER]={POWER_BUTTON};


void Power_Sequence_Struct_Init(void){
  Power.system_boot = 0;
  Power.Power_Button = 0;
  Power.Power_Mode = 0;
  Power.app_reboot = 0;
  Power.factory_reset = 0;
}
power_t Power;


typedef struct button_t{
  uint8_t  Result[TOTAL_BUTTON_NUMBER];
  uint8_t  TriggerMethod[TOTAL_BUTTON_NUMBER];
  uint8_t  LastState[TOTAL_BUTTON_NUMBER];
  uint32_t PressDuration[TOTAL_BUTTON_NUMBER];
  uint32_t PressDurationTemp[TOTAL_BUTTON_NUMBER];
  uint8_t  LongPressFlag[TOTAL_BUTTON_NUMBER];
  uint8_t  UsingReferenceTimer;
  uint8_t  ErrorCode;
}button_t;
button_t Button;

void Button_Struct_Init(void){
	for(uint8_t i=0;i<TOTAL_BUTTON_NUMBER;i++){
    Button.Result[i]=0;
    Button.TriggerMethod[i]=0;
    Button.LastState[i]=0;
    Button.PressDuration[i]=0;
    Button.PressDurationTemp[i]=0;
    Button.LongPressFlag[i]=0;
  }
  Button.UsingReferenceTimer=0;
  Button.ErrorCode=0;
}

void Power_Button_GPIO_Init(uint8_t mode){    //1-input 2-output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << POWER_BUTTON),
        .mode = mode,
        .pull_up_en = GPIO_PULLUP_ENABLE,     // enable if button to GND
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE        // no interrupt (polling)
    };
    gpio_config(&io_conf);

    esp_rom_gpio_pad_select_gpio(POWER_CTRL); 
    gpio_reset_pin(POWER_CTRL);
    gpio_set_direction (POWER_CTRL, GPIO_MODE_OUTPUT);
}

void CHRG_En_GPIO_Init(uint8_t mode){         //1-input 2-output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << 17),
        .mode = mode,
        .pull_up_en = GPIO_PULLUP_ENABLE,     // enable if button to GND
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE        // no interrupt (polling)
    };
    gpio_config(&io_conf);
}
void CHRG_Sts_GPIO_Init(uint8_t mode){        //1-input 2-output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << 9),
        .mode = mode,
        .pull_up_en = GPIO_PULLUP_ENABLE,     // enable if button to GND
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE        // no interrupt (polling)
    };
    gpio_config(&io_conf);
}
void Charger_Connect_Sts_GPIO_Init(uint8_t mode){        //1-input 2-output
    gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << 14),
        .mode = mode,
        .pull_up_en = GPIO_PULLUP_DISABLE,     // enable if button to GND
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE         // no interrupt (polling)
    };
    gpio_config(&io_conf);
}

uint8_t charge_en = 0;
void Battery_Charge_Enable (uint8_t val){
  if(val == 1){
    gpio_set_level(17, 1);
    charge_en = 1;
  }
  else{
    gpio_set_level(17, 0);
    charge_en = 0;
  }
}

uint8_t Get_Battery_Charge_Enable_Sts (void){
  return charge_en;
}
uint8_t Get_Battery_Charge_Sts (void){
  return gpio_get_level(9);
}
uint8_t Get_Charger_Connect_Sts (void){
  // return 0; // debug purpose
  return gpio_get_level(14);
}

void Button_Response(uint8_t button){
  if(gpio_get_level((Button_Pins[button]))){
    Button.Result[button]=0;
  }else{
    Button.Result[button]=1;
  }
}

void Button_Check_Trigger(uint8_t button){
  uint8_t current_state=0;
  Button_Response(button);
  current_state=Button.Result[button];
  if( (Button.LastState[button] == 0) && (current_state == 1) ){
      Button.LongPressFlag[button]=0x00;  //Rising Edge
      Button.TriggerMethod[button]=0x01;
  }else if( (Button.LastState[button] == 1) && (current_state==0)){
      Button.TriggerMethod[button]=0x04;  //Falling Edge
  }else if( (Button.LastState[button] == 1) && (current_state==1)){
      Button.TriggerMethod[button]=0x02;  //Constant Touched
  }else{
      Button.TriggerMethod[button]=0x00;  //No Touch Event Occured
  }
  Button.LastState[button]=current_state;
}

void Button_Check_Press_Duration(uint32_t reference_time, uint8_t button){
  uint32_t duration=0;
  Button_Check_Trigger(button);
  if(Button.TriggerMethod[button]==0x01){
    Button.UsingReferenceTimer=1;
    Button.PressDurationTemp[button]=reference_time;
  }else if( (Button.TriggerMethod[button]==0x04) || (Button.TriggerMethod[button]==0x02) ){
    duration=reference_time;
    duration-=Button.PressDurationTemp[button];
    if(duration>0){
        Button.PressDuration[button]=duration;
    }
    if(Button.TriggerMethod[button]==0x04){
      Button.PressDurationTemp[button]=reference_time;
      Button.UsingReferenceTimer=0;
    }
  }
  else{
    Button.PressDuration[button]=0;
    Button.PressDurationTemp[button]=0;
  }
}

uint8_t Button_Short_Pressed(uint8_t button){
  uint8_t sts=0;
  if(Button.TriggerMethod[button]==0x04){
    if((Button.PressDuration[button] > BUTTON_SHORT_PRESS_TIME_MIN ) && ( Button.PressDuration[button] < BUTTON_SHORT_PRESS_TIME_MAX)){
      sts=1;
    }
  }
  return sts;
}


/* ---- Several short presses: Button_Short_Pressed_Times(2), (3) ... ------------------------------------------
 * Every short press of the button is counted. The count is final when no new press came for
 * BUTTON_MULTI_GAP_MS, or BUTTON_MULTI_WINDOW_MS (3 s) after the first press - so 2 presses are not mistaken for
 * the start of 3. Button_Short_Pressed_Times(n) then returns 1 ONCE when exactly n presses were counted
 * (so the action comes about BUTTON_MULTI_GAP_MS after the last press). Needs Button_Scan() on every loop pass. */
#define BUTTON_MULTI_WINDOW_MS   3000   /* all presses must be inside this time, counted from the first press */
#define BUTTON_MULTI_GAP_MS      800    /* the sequence ends when no press came for this long */
#define BUTTON_MULTI_VALID_MS    500    /* a finished count waits this long to be asked for, then it is dropped */

static uint8_t  multi_count = 0;        /* presses counted in the running sequence */
static uint32_t multi_first = 0;        /* time of the first press */
static uint32_t multi_last = 0;         /* time of the latest press */
static uint8_t  multi_result = 0;       /* finished count (0 = none waiting) */
static uint32_t multi_result_time = 0;

static void Button_Multi_Update(uint32_t reference_time){
  if(multi_count > 0 && ((reference_time - multi_last) >= BUTTON_MULTI_GAP_MS || (reference_time - multi_first) >= BUTTON_MULTI_WINDOW_MS)){
    multi_result = multi_count;         /* the sequence is over */
    multi_result_time = reference_time;
    multi_count = 0;
  }
  if(multi_result > 0 && (reference_time - multi_result_time) > BUTTON_MULTI_VALID_MS){
    multi_result = 0;                   /* nobody asked for this count */
  }
  if(Button_Short_Pressed(0)){
    if(multi_count == 0){
      multi_first = reference_time;
    }
    multi_count++;
    multi_last = reference_time;
  }
}

uint8_t Button_Short_Pressed_Times(uint8_t times){
  if(times > 0 && multi_result == times){
    multi_result = 0;
    return 1;
  }
  return 0;
}

void Button_Scan(uint32_t reference_time){
	for(uint8_t i=0;i<TOTAL_BUTTON_NUMBER;i++){
		Button_Check_Press_Duration(reference_time,i);
	}
	Button_Multi_Update(reference_time);
}

uint8_t Button_Long_Pressed(uint8_t button){
  uint8_t sts=0;
  if((Button.PressDuration[button]> BUTTON_LONG_PRESS_TIME) && (Button.LongPressFlag[button]==0)){
      Button.LongPressFlag[button]=1;
    sts=1;
  }
  return sts;
}

uint8_t Push_Button_Get_Reference_Timer_Use_Flag(void){
  return Button.UsingReferenceTimer;
}

uint8_t get_Button_Response (uint8_t button){
	return Button.Result[button];
}




#define TIMER_4_INTERVAL_MILI_SEC      1

volatile uint64_t  Timer_Counter_4 = 0;
esp_timer_handle_t Periodic_Timer_4;



////////////////////////////////////@ESP-32 Hardware GP Timers//////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////
///---------------------------------Captouch Timer------------------------------------///
void IRAM_ATTR Timer_Callback_4(void* arg){
    Timer_Counter_4++;
}
uint64_t ESP_Timer_4_Get_Time (void){
   return Timer_Counter_4;
}
void ESP_Timer_4_Start (void){
    // Configure the Timer
    esp_timer_create_args_t Timer_Args_4 = {
        .callback = &Timer_Callback_4,
        .name = "Periodic_Timer_4"
    };
    esp_timer_create(&Timer_Args_4, &Periodic_Timer_4);
    // Start the Timer to fire every 1 miliseconds
    esp_timer_start_periodic(Periodic_Timer_4, TIMER_4_INTERVAL_MILI_SEC * 1000);
}
void Reset_ESP_Timer_4(void) {
    // Stop the Timer
    esp_timer_stop(Periodic_Timer_4);
    // Reset the counter
    Timer_Counter_4 = 0;
    // Start the Timer again
    esp_timer_start_periodic(Periodic_Timer_4, TIMER_4_INTERVAL_MILI_SEC * 1000);
}
