#ifndef QMA6100P_H
#define QMA6100P_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// 1. Core accelerometer reading type
typedef struct {
    float x;
    float y;
    float z;
} qma_accel_data_t;

// 2. Orientation states (roll left/right, flat up/down)
typedef enum {
    QMA_ORIENT_UNKNOWN = 0,
    QMA_ORIENT_NORMAL,       // your default/handheld resting pose
    QMA_ORIENT_FLAT_UP,      // face up, resting flat
    QMA_ORIENT_FLAT_DOWN,    // face down, resting flat
    QMA_ORIENT_LEFT_SIDE,    // rolled/rotated onto its left side
    QMA_ORIENT_RIGHT_SIDE    // rolled/rotated onto its right side
} qma_orientation_t;

// 3. Shake intensity levels
typedef enum {
    QMA_SHAKE_LIGHT = 0,
    QMA_SHAKE_MEDIUM,
    QMA_SHAKE_HARD
} qma_shake_level_t;

// 4. Expose the core library functions to main.c
void      QMA6100P_Init(void);
uint8_t   QMA6100P_Get_Interrupt(void);
esp_err_t QMA6100P_Get_Accel_Continuous(qma_accel_data_t *accel);
void      QMA6100P_Debug_Print_Accel(void);
void      QMA6100P_Debug_Live_Stream(int iterations);
void      QMA6100P_Debug_Print_Accel_Avg(int samples);

void      QMA6100P_INT1_Gpio_Init(void);

qma_orientation_t QMA6100P_Get_Orientation(void);
qma_orientation_t QMA6100P_Get_Confirmed_Orientation(uint32_t hold_ms);
const char       *QMA6100P_Orientation_Str(qma_orientation_t o);
qma_shake_level_t QMA6100P_Get_Shake_Intensity(float *out_peak_g);
bool              QMA6100P_Poll_Shake(qma_shake_level_t *out_level, float *out_peak_g);


#endif // QMA6100P_H
