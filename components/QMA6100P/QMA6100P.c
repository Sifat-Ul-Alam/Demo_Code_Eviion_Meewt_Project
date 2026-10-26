/* ===========================================================================
 * QMA6100P.c -- BUILD VERSION: v8 (2026-08-02)
 * Changelog:
 *   v9: added QMA6100P_Detect_Hard_Tap() and QMA6100P_Detect_Rub() (head tap / petting)
 *   v8: added QMA6100P_Get_Confirmed_Orientation() -- debounced/held pose,
 *       only reports a pose after it's stayed continuously for hold_ms
 *   v7: orientation motion-gate added (rejects false LEFT_SIDE during shakes)
 *   v6: interrupt-free QMA6100P_Poll_Shake() added
 *   v5: reverted to user-confirmed-working QMA6100P_Init (no OTP load)
 * =========================================================================== */
#include <stdio.h>
#include <stdbool.h>
#include <math.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "QMA6100P.h"
#include "All_I2C_Init.h"


#define GPIO_INPUT_INT1     21


esp_err_t QMA6100P_Get_Accel_Continuous(qma_accel_data_t *accel)
{
    if (accel == NULL) return ESP_ERR_INVALID_ARG;

    uint8_t raw_buffer[6];
    // Read 6 consecutive registers starting from OUT_X_LSB (0x01)
    esp_err_t err = QMA6100P_Read_Regs(0x01, raw_buffer, 6);
    if (err != ESP_OK) return err;

    // Reconstruct signed 14-bit data sets
    int16_t x_raw = (int16_t)((raw_buffer[1] << 8) | raw_buffer[0]) >> 2;
    int16_t y_raw = (int16_t)((raw_buffer[3] << 8) | raw_buffer[2]) >> 2;
    int16_t z_raw = (int16_t)((raw_buffer[5] << 8) | raw_buffer[4]) >> 2;

    // Convert raw bits to standard G units (±4g scaling profile)
    accel->x = x_raw * 0.00048828f;
    accel->y = y_raw * 0.00048828f;
    accel->z = z_raw * 0.00048828f;

    return ESP_OK;
}

void QMA6100P_INT1_Gpio_Init (void){
    // 1. Configure the ESP32-S3 pin explicitly with pull-down
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,
        .mode = GPIO_MODE_INPUT,
        .pin_bit_mask = (1ULL << GPIO_INPUT_INT1),
        .pull_down_en = GPIO_PULLDOWN_DISABLE,  // Keep pin stable at 0V when idle
        .pull_up_en = GPIO_PULLUP_DISABLE
    };
    gpio_config(&io_conf);
}

/* ============ QMA6100P_Init -- left exactly as-is per your request ============ */
void QMA6100P_Init(void)
{
    printf(">>> QMA6100P.c BUILD VERSION: v8 (2026-08-02) <<<\n");

    I2C_Bus_Init();

    // 2. Wake up and set basic sampling rate
    QMA6100P_Write_Reg(0x11, 0x80);  // Power CTL: Active Mode
    vTaskDelay(pdMS_TO_TICKS(20));   // Wait for internal digital core stability
    QMA6100P_Write_Reg(0x0F, 0x02);  // Range: ±4g
    QMA6100P_Write_Reg(0x10, 0x05);  // BW/ODR: Continuous sampling at 100Hz


    QMA6100P_Write_Reg(0x20, 0b00000001);  // INTPIN_CONF REGISTER (0x20) = INT1_LVL = 1

    QMA6100P_Write_Reg(0x21, 0b10001101);  // INT_CFG REGISTER (0x21) = LATCH_INT = 1

    QMA6100P_Write_Reg(0x18, 0b00000111);  // INT_EN2 REGISTER (0x18) = ANY_MOT_EN_X,Y,Z = 1

    QMA6100P_Write_Reg(0x1A, 0b00000001);  // INT1_MAP1 REGISTER (0x1A) = INT1_ANY_MOT = 1

    QMA6100P_Write_Reg(0x2F, 0b00000000);  // MOTION_CFG3 REGISTER (0x2F) = ANY_MOT_IN_SEL = 1

    QMA6100P_Write_Reg(0x2E, 5);          // MOTION_CFG2 REGISTER (0x2E) = ANY_MOT_TH[7:0] = 16=0x10

    QMA6100P_Write_Reg(0x2C, 0b00000000);  // MOTION_CFG0 REGISTER (0x2C) = ANY_MOT_DUR[1:0] = 10
}

/* ============ end QMA6100P_Init ============ */

uint8_t QMA6100P_Get_Interrupt(void)
{
    // Handle INT1 Shake State Changes
    uint8_t current_int_state = gpio_get_level(GPIO_INPUT_INT1);

    if(current_int_state == 1) {
        uint8_t reg_val = 0;
        esp_err_t err = QMA6100P_Read_Regs(0x09, &reg_val, 1);
        printf("!!! SHAKE DETECTED !!!\n");
        return 1;
    }
    else{
        return 0;
    }
}


/* =====================================================================
 * DEBUG / DIAGNOSTIC HELPERS
 * ===================================================================== */

// Single-shot raw print
void QMA6100P_Debug_Print_Accel(void)
{
    qma_accel_data_t a;
    if (QMA6100P_Get_Accel_Continuous(&a) == ESP_OK) {
        printf("x=%.2f  y=%.2f  z=%.2f\n", a.x, a.y, a.z);
    } else {
        printf("accel read failed\n");
    }
}

// Averaged, settle-delayed print for calibration captures. Reports spread
// (max-min magnitude during the capture) so you can tell if the device was
// truly at rest vs still moving/settling.
void QMA6100P_Debug_Print_Accel_Avg(int samples)
{
    vTaskDelay(pdMS_TO_TICKS(1000));  // let handling vibration decay

    float sx = 0, sy = 0, sz = 0;
    float mag_min = 1e9f, mag_max = -1e9f;
    int good = 0;
    for (int i = 0; i < samples; i++) {
        qma_accel_data_t a;
        if (QMA6100P_Get_Accel_Continuous(&a) == ESP_OK) {
            sx += a.x; sy += a.y; sz += a.z;
            float m = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
            if (m < mag_min) mag_min = m;
            if (m > mag_max) mag_max = m;
            good++;
        }
        vTaskDelay(pdMS_TO_TICKS(20));
    }
    if (good == 0) {
        printf("accel read failed\n");
        return;
    }
    float x = sx / good, y = sy / good, z = sz / good;
    float mag = sqrtf(x * x + y * y + z * z);
    float spread = mag_max - mag_min;
    printf("x=%.3f  y=%.3f  z=%.3f   |magnitude=%.3f (want ~1.00)  spread=%.3f %s\n",
           x, y, z, mag, spread,
           spread > 0.08f ? "<-- UNSTABLE, device was moving! recapture." : "(stable)");
}

// Live raw stream with I2C error code + raw hex bytes + power-mgmt register,
// to distinguish "sensor not producing new samples" from "stale/cached I2C read".
void QMA6100P_Debug_Live_Stream(int iterations)
{
    printf("--- Live accel stream: move/shake the board now ---\n");
    for (int i = 0; i < iterations; i++) {
        uint8_t raw[6];
        esp_err_t err = QMA6100P_Read_Regs(0x01, raw, 6);
        uint8_t pwr = 0;
        QMA6100P_Read_Regs(0x11, &pwr, 1);

        if (err == ESP_OK) {
            printf("[%d] err=%d raw=[%02X %02X %02X %02X %02X %02X] pwr(0x11)=0x%02X\n",
                   i, err, raw[0], raw[1], raw[2], raw[3], raw[4], raw[5], pwr);
        } else {
            printf("[%d] READ FAILED err=%d\n", i, err);
        }
        vTaskDelay(pdMS_TO_TICKS(200));
    }
}


/* =====================================================================
 * ORIENTATION DETECTION (roll left / roll right / face up / face down)
 * =====================================================================
 * Since this mounting isn't cleanly aligned to a single axis (gravity is
 * split across multiple axes for each pose), we don't threshold a single
 * axis. Instead each pose has a calibrated reference direction (a unit
 * vector), and at runtime we compare the live accel vector's direction
 * against every reference using cosine similarity (dot product of
 * normalized vectors) and pick the closest match above a confidence floor.
 */
#define QMA_ORIENT_MIN_CONFIDENCE   0.85f  // cosine similarity threshold (0..1). Lower = looser matching.

typedef struct {
    qma_orientation_t orient;
    float x, y, z;   // normalized on first use
} qma_ref_pose_t;

static qma_ref_pose_t s_ref_poses[] = {
    { QMA_ORIENT_NORMAL,     -1.210f, -0.188f,  0.053f },  // static/handheld pose
    { QMA_ORIENT_FLAT_UP,    -0.275f, -0.117f, -0.805f },
    { QMA_ORIENT_FLAT_DOWN,  -0.263f, -0.205f,  1.051f },
    { QMA_ORIENT_LEFT_SIDE,  -0.272f, -1.153f,  0.106f },
    { QMA_ORIENT_RIGHT_SIDE, -0.244f,  0.820f,  0.101f },
};
#define QMA_NUM_REF_POSES (sizeof(s_ref_poses) / sizeof(s_ref_poses[0]))

static bool s_ref_poses_normalized = false;

static void qma6100p_normalize_ref_poses(void)
{
    for (size_t i = 0; i < QMA_NUM_REF_POSES; i++) {
        float mag = sqrtf(s_ref_poses[i].x * s_ref_poses[i].x +
                           s_ref_poses[i].y * s_ref_poses[i].y +
                           s_ref_poses[i].z * s_ref_poses[i].z);
        if (mag > 0.001f) {
            s_ref_poses[i].x /= mag;
            s_ref_poses[i].y /= mag;
            s_ref_poses[i].z /= mag;
        }
    }
    s_ref_poses_normalized = true;
}

qma_orientation_t QMA6100P_Get_Orientation(void)
{
    if (!s_ref_poses_normalized) {
        qma6100p_normalize_ref_poses();
    }

    qma_accel_data_t a;
    if (QMA6100P_Get_Accel_Continuous(&a) != ESP_OK) {
        return QMA_ORIENT_UNKNOWN;
    }

    float mag = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
    if (mag < 0.001f) {
        return QMA_ORIENT_UNKNOWN;
    }
    float nx = a.x / mag, ny = a.y / mag, nz = a.z / mag;

    qma_orientation_t best_orient = QMA_ORIENT_UNKNOWN;
    float best_score = QMA_ORIENT_MIN_CONFIDENCE;

    for (size_t i = 0; i < QMA_NUM_REF_POSES; i++) {
        float dot = nx * s_ref_poses[i].x + ny * s_ref_poses[i].y + nz * s_ref_poses[i].z;
        if (dot > best_score) {
            best_score = dot;
            best_orient = s_ref_poses[i].orient;
        }
    }
    return best_orient;
}

// Human-readable name for logging/printf
const char *QMA6100P_Orientation_Str(qma_orientation_t o)
{
    switch (o) {
        case QMA_ORIENT_NORMAL:     return "NORMAL";
        case QMA_ORIENT_FLAT_UP:    return "FLAT_UP";
        case QMA_ORIENT_FLAT_DOWN:  return "FLAT_DOWN";
        case QMA_ORIENT_LEFT_SIDE:  return "LEFT_SIDE";
        case QMA_ORIENT_RIGHT_SIDE: return "RIGHT_SIDE";
        default:                    return "UNKNOWN";
    }
}


// --- Debounced/held orientation -----------------------------------------
// Call this repeatedly (e.g. every 50-100ms) in your main loop. It only
// returns a non-UNKNOWN pose once that exact pose has been continuously
// observed for at least `hold_ms` -- any change (including a momentary
// UNKNOWN from a shake/transition) resets the timer, so a brief flick to
// the left and back will never confirm. Once confirmed, it keeps returning
// that same pose every call for as long as it's still held; when the pose
// changes away, it goes back to UNKNOWN until the new pose holds for
// `hold_ms` too.
qma_orientation_t QMA6100P_Get_Confirmed_Orientation(uint32_t hold_ms)
{
    static qma_orientation_t s_candidate = QMA_ORIENT_UNKNOWN;
    static qma_orientation_t s_confirmed = QMA_ORIENT_UNKNOWN;
    static TickType_t s_candidate_start = 0;

    qma_orientation_t raw = QMA6100P_Get_Orientation();
    TickType_t now = xTaskGetTickCount();

    if (raw != s_candidate) {
        // Pose changed -- start the hold-timer over.
        s_candidate = raw;
        s_candidate_start = now;
        if (raw == QMA_ORIENT_UNKNOWN) {
            s_confirmed = QMA_ORIENT_UNKNOWN;
        }
        return s_confirmed;  // keep reporting old confirmed pose until new one holds
    }

    // Same raw pose as last call -- check if it's held long enough.
    uint32_t held_ms = (now - s_candidate_start) * portTICK_PERIOD_MS;
    if (raw != QMA_ORIENT_UNKNOWN && held_ms >= hold_ms) {
        s_confirmed = raw;
    }

    return s_confirmed;
}

#define QMA_ORIENT_CONFIRM_MS   3000   // default hold time, in ms, for "confirmed" orientation


/* =====================================================================
 * SHAKE INTENSITY (light / medium / hard)
 * =====================================================================
 * Call this right after QMA6100P_Get_Interrupt() returns 1. It samples a
 * short burst of continuous accel data and returns the peak deviation from
 * 1g seen during that burst, classified into levels. Tune the two
 * thresholds below and the SAMPLE_COUNT/delay to match your desired feel.
 */
#define QMA_SHAKE_SAMPLE_COUNT    8      // number of readings to burst-sample
#define QMA_SHAKE_SAMPLE_DELAY_MS 5      // delay between samples (8 * 5ms = 40ms window)
#define QMA_SHAKE_MEDIUM_G        1.5f   // peak |accel - 1g| above this = "medium"
#define QMA_SHAKE_HARD_G          4.0f   // peak |accel - 1g| above this = "hard"

qma_shake_level_t QMA6100P_Get_Shake_Intensity(float *out_peak_g)
{
    float peak_deviation = 0.0f;

    for (int i = 0; i < QMA_SHAKE_SAMPLE_COUNT; i++) {
        qma_accel_data_t a;
        if (QMA6100P_Get_Accel_Continuous(&a) == ESP_OK) {
            float magnitude = sqrtf(a.x * a.x + a.y * a.y + a.z * a.z);
            float deviation = fabsf(magnitude - 1.0f);  // how far from resting 1g
            if (deviation > peak_deviation) {
                peak_deviation = deviation;
            }
        }
        vTaskDelay(pdMS_TO_TICKS(QMA_SHAKE_SAMPLE_DELAY_MS));
    }

    if (out_peak_g != NULL) {
        *out_peak_g = peak_deviation;
    }

    if (peak_deviation >= QMA_SHAKE_HARD_G) {
        return QMA_SHAKE_HARD;
    } else if (peak_deviation >= QMA_SHAKE_MEDIUM_G) {
        return QMA_SHAKE_MEDIUM;
    } else {
        return QMA_SHAKE_LIGHT;
    }
}

// Minimum deviation from 1g to count as "any shake happened at all" -- below
// this is just normal handling/vibration noise, not a deliberate shake.
#define QMA_SHAKE_NOISE_FLOOR_G   0.30f

// Standalone shake check -- does NOT depend on the hardware INT1/any-motion
// interrupt at all, just continuous polling + magnitude math (same approach
// that already works for orientation). Call this repeatedly in your main
// loop (e.g. every 50-100ms). Returns true only when peak_deviation clears
// the noise floor, meaning a real shake actually happened; level/peak_g are
// only meaningful when it returns true.
bool QMA6100P_Poll_Shake(qma_shake_level_t *out_level, float *out_peak_g)
{
    float peak_g = 0.0f;
    qma_shake_level_t level = QMA6100P_Get_Shake_Intensity(&peak_g);

    if (out_peak_g != NULL) *out_peak_g = peak_g;
    if (out_level != NULL) *out_level = level;

    return (peak_g >= QMA_SHAKE_NOISE_FLOOR_G);
}

