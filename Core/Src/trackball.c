#include "trackball.h"
#include "keyboard_state.h"
#include "keymaps.h"
#include "hid_mouse.h"
#include "ratemeter.h"
#include "glider.h"
#include "math_utils.h"
#include "config.h"
#include "prec_time.h"
#include "main.h"
#include "stm32f1xx_hal.h"
#include "stm32f1xx.h"
#include "usbd_custom_hid_if.h"
#include <math.h>
#include <stdint.h>

typedef enum {
    AXIS_X,
    AXIS_Y,
    AXIS_NUM
} Axis;

static volatile int8_t distances[AXIS_NUM];
static RateMeter ratemeter[AXIS_NUM];
static float pointer_buffer[AXIS_NUM];
static float wheel_buffer[AXIS_NUM]; /* Accumulates encoder ticks; int16_t to avoid overflow */
static bool as_wheel = false;
static bool last_wheel_node = false;

// Trackball configuration parameters for each layer
static float config_acceleration_exponent[LAYERS_NUM] = {0};
static float config_acceleration_divisor[LAYERS_NUM] = {0};
static float config_scroll_vertical_exponent[LAYERS_NUM] = {0};
static float config_scroll_vertical_divisor[LAYERS_NUM] = {0};
static float config_scroll_horizontal_exponent[LAYERS_NUM] = {0};
static float config_scroll_horizontal_divisor[LAYERS_NUM] = {0};

// Trackball configuration parameters for the current layer
static float acceleration_exponent;
static float acceleration_divisor;
static float scroll_vertical_exponent;
static float scroll_vertical_divisor;
static float scroll_horizontal_exponent;
static float scroll_horizontal_divisor;

#if VARIANT_TRACKBALL_GLIDE
/* The pointer glides in steps of this length, like the main loop of the stock firmware */
#define GLIDE_STEP_MS 8
/* Pointer speed of the stock firmware: pixels per millisecond for one pulse per second */
#define GLIDE_STOCK_GAIN (1.0f / 30.0f)

static Glider glider[AXIS_NUM];
static uint32_t glide_time_us;

/* Stored with an offset of 1, because 0 means "not set on this layer" */
static float config_smoothness[LAYERS_NUM] = {0};
static uint8_t config_measurement[LAYERS_NUM] = {0};

/* Sustain time in relation to the stock firmware, 1 = the same */
static float smoothness;
#endif

static int8_t apply_acceleration(int8_t steps, float rate, float exponent, float divisor)
{
    if (steps == 0) {
        return 0;
    }
    if (divisor <= 0.0f) {
        return steps;
    }
    const float factor = 1.0f + powf(rate, exponent) / divisor;
    const float scaled = fabsf((float)steps) * factor;
    const int8_t magnitude = clamp_int8((int32_t)roundf(scaled));
    return (steps > 0) ? magnitude : (int8_t)(-magnitude);
}

#if !VARIANT_TRACKBALL_GLIDE
/* Vector-based acceleration: uses the magnitude of the (dx, dy) vector and the
   combined rate so the acceleration factor is the same for both axes. This
   preserves the direction of motion exactly (no per-axis distortion) and makes
   the cursor speed for diagonal movement match the speed for axis-aligned
   movement at the same ball speed. */
static void apply_vector_acceleration(float dx, float dy,
                                      float rate_x, float rate_y,
                                      float exponent, float divisor,
                                      float* out_x, float* out_y)
{
    if (dx == 0.0f && dy == 0.0f) {
        *out_x = 0.0f;
        *out_y = 0.0f;
        return;
    }
    if (divisor <= 0.0f) {
        *out_x = dx;
        *out_y = dy;
        return;
    }
    const float combined_rate = hypot_f(rate_x, rate_y);
    const float factor = 1.0f + powf(combined_rate, exponent) / divisor;
    *out_x = dx * factor;
    *out_y = dy * factor;
}
#endif

#if VARIANT_TRACKBALL_GLIDE

/* A pulse has arrived on this axis: set the speed of the pointer.
   The speed is a vector, so the other axis gets its share of it. */
static void glide_pulse(Axis axis, int8_t direction)
{
    const Axis other = (axis == AXIS_X) ? AXIS_Y : AXIS_X;
    float rate[AXIS_NUM];

    glider_set_direction(&glider[axis], direction);

    rate[AXIS_X] = ratemeter_rate(&ratemeter[AXIS_X]);
    rate[AXIS_Y] = ratemeter_rate(&ratemeter[AXIS_Y]);
    const float combined_rate = hypot_f(rate[AXIS_X], rate[AXIS_Y]);
    if (combined_rate <= 0.0f || acceleration_divisor <= 0.0f) {
        return;
    }

    /* TRACKBALL_SPEED(100) and TRACKBALL_ACCELERATION(0) are the stock firmware */
    const float gain = GLIDE_STOCK_GAIN * (1000.0f / acceleration_divisor) / 100.0f;
    const float speed = gain * powf(combined_rate, acceleration_exponent);
    const float ratio = speed / combined_rate;

    const float interval_ms = (float)ratemeter[axis].averageDelta / 1000.0f;
    const float sustain = smoothness * sqrtf(interval_ms);

    glider_update(&glider[axis], rate[axis] * ratio, (uint16_t)((sustain < 1000.0f) ? sustain : 1000.0f));
    glider_update_speed(&glider[other], rate[other] * ratio);
}

/* Moves the pointer along, the distance is added to pointer_buffer */
static void glide_step(uint32_t time_delta_us)
{
    glide_time_us += time_delta_us;
    if (glide_time_us < GLIDE_STEP_MS * 1000UL) {
        return;
    }
    glide_time_us -= GLIDE_STEP_MS * 1000UL;
    if (glide_time_us > GLIDE_STEP_MS * 1000UL) {
        glide_time_us = 0;
    }

    const GlideResult rx = glider_glide(&glider[AXIS_X], GLIDE_STEP_MS);
    const GlideResult ry = glider_glide(&glider[AXIS_Y], GLIDE_STEP_MS);
    pointer_buffer[AXIS_X] += rx.value;
    pointer_buffer[AXIS_Y] += ry.value;
    /* The stock firmware stops the Y axis when the X axis has come to rest,
       but not the other way around */
    if (rx.stopped) {
        glider_stop(&glider[AXIS_Y]);
    }
}

static void glide_reset(void)
{
    glider_init(&glider[AXIS_X]);
    glider_init(&glider[AXIS_Y]);
    glide_time_us = 0;
}

#endif

// Interrupt handlers
void trackball_interrupt_x_neg(void)
{
    distances[AXIS_X] -= 1;
    ratemeter_onInterrupt(&ratemeter[AXIS_X]);
}

void trackball_interrupt_x_pos(void)
{
    distances[AXIS_X] += 1;
    ratemeter_onInterrupt(&ratemeter[AXIS_X]);
}

void trackball_interrupt_y_neg(void)
{
    distances[AXIS_Y] -= 1;
    ratemeter_onInterrupt(&ratemeter[AXIS_Y]);
}

void trackball_interrupt_y_pos(void)
{
    distances[AXIS_Y] += 1;
    ratemeter_onInterrupt(&ratemeter[AXIS_Y]);
}

USBD_StatusTypeDef trackball_task(void)
{
    int8_t x = 0, y = 0, w = 0, hw = 0;
    int8_t move_delta[AXIS_NUM];
    const uint32_t time_delta = PREC_TIME_DELTA_US();
    const float rate[AXIS_NUM] = {[AXIS_X] = ratemeter_rate(&ratemeter[AXIS_X]), [AXIS_Y] = ratemeter_rate(&ratemeter[AXIS_Y])};

    // Use wheel mode (Fn + trackball)
    as_wheel = keyboard_state.fn;

    if (time_delta == 0) {
        return USBD_OK; // Skip first iteration to get proper delta next time
    }
    
    ATOM_MOVE(move_delta[AXIS_X], distances[AXIS_X]);
    ATOM_MOVE(move_delta[AXIS_Y], distances[AXIS_Y]);

    // Reset when switching modes
    if (as_wheel != last_wheel_node) {
        ratemeter_init(&ratemeter[AXIS_X]);
        ratemeter_init(&ratemeter[AXIS_Y]);
        wheel_buffer[AXIS_X] = 0;
        wheel_buffer[AXIS_Y] = 0;
        pointer_buffer[AXIS_X] = 0;
        pointer_buffer[AXIS_Y] = 0;
        move_delta[AXIS_X] = 0;
        move_delta[AXIS_Y] = 0;
        last_wheel_node = as_wheel;
#if VARIANT_TRACKBALL_GLIDE
        glide_reset();
#endif
    }

    ratemeter_tick(&ratemeter[AXIS_X], time_delta);
    ratemeter_tick(&ratemeter[AXIS_Y], time_delta);

    if (as_wheel) {        
        // Vertical scroll (wheel) - Y axis
        wheel_buffer[AXIS_Y] += apply_acceleration(
            (int32_t)move_delta[AXIS_Y],
            rate[AXIS_Y],
            scroll_vertical_exponent,
            scroll_vertical_divisor) 
            * (VERTICAL_SCROLL_INVERTED ? 1 : -1);
        w = clamp_int8((int32_t)wheel_buffer[AXIS_Y]);
        
        // Horizontal scroll (pan) - X axis
        wheel_buffer[AXIS_X] += apply_acceleration(
            move_delta[AXIS_X],
            rate[AXIS_X],
            scroll_horizontal_exponent,
            scroll_horizontal_divisor)
            * (HORIZONTAL_SCROLL_INVERTED ? -1 : 1);
        hw = clamp_int8((int32_t)wheel_buffer[AXIS_X]);
    } else {
#if VARIANT_TRACKBALL_GLIDE
        // Pointer movement - every pulse sets the speed, the pointer glides
        if (move_delta[AXIS_X] != 0) {
            glide_pulse(AXIS_X, sign(move_delta[AXIS_X]));
        }
        if (move_delta[AXIS_Y] != 0) {
            glide_pulse(AXIS_Y, sign(move_delta[AXIS_Y]));
        }
        glide_step(time_delta);
#else
        // Pointer movement - vector-based acceleration so that the direction of
        // motion is preserved (per-axis acceleration distorts diagonals because
        // a non-linear curve is applied to two different per-axis rates).
        float scaled_x = 0.0f;
        float scaled_y = 0.0f;
        apply_vector_acceleration(
            (float)move_delta[AXIS_X], (float)move_delta[AXIS_Y],
            rate[AXIS_X], rate[AXIS_Y],
            acceleration_exponent, acceleration_divisor,
            &scaled_x, &scaled_y);
        pointer_buffer[AXIS_X] += scaled_x;
        pointer_buffer[AXIS_Y] += scaled_y;
#endif
        x = clamp_int8((int32_t)pointer_buffer[AXIS_X]);
        y = clamp_int8((int32_t)pointer_buffer[AXIS_Y]);
    }

    if (x != 0 || y != 0 || w != 0 || hw != 0) {
        #if KEYBOARD_BACKLIGHT_RESUME_BY_TRACKBALL
        keyboard_state.last_activity_time = HAL_GetTick();
        #endif        
        USBD_StatusTypeDef result = hid_mouse_move(x, y, w, hw);
        if (result == USBD_OK) {
            pointer_buffer[AXIS_X] -= x;
            pointer_buffer[AXIS_Y] -= y;
            wheel_buffer[AXIS_X] -= hw;
            wheel_buffer[AXIS_Y] -= w;
        }
        return result;
    }

    return USBD_OK;
}

void trackball_load_layer_config(void)
{
    #define LOAD_CONFIG(name, default_value) \
        name = config_##name[keyboard_state.layer]; \
        if (!name && keyboard_state.layer > 0) { \
            name = config_##name[0]; \
        } \
        if (!name) name = default_value;

    LOAD_CONFIG(acceleration_exponent, 1.0f + DEFAULT_TRACKBALL_ACCELERATION);
    LOAD_CONFIG(acceleration_divisor, 1000.0f / DEFAULT_TRACKBALL_SPEED);
    LOAD_CONFIG(scroll_vertical_exponent, 1.0f + DEFAULT_TRACKBALL_SCROLL_VERTICAL_ACCELERATION);
    LOAD_CONFIG(scroll_vertical_divisor, 10000.0f / DEFAULT_TRACKBALL_SCROLL_VERTICAL_SPEED);
    LOAD_CONFIG(scroll_horizontal_exponent, 1.0f + DEFAULT_TRACKBALL_SCROLL_HORIZONTAL_ACCELERATION);
    LOAD_CONFIG(scroll_horizontal_divisor, 10000.0f / DEFAULT_TRACKBALL_SCROLL_HORIZONTAL_SPEED);

    #undef LOAD_CONFIG

#if VARIANT_TRACKBALL_GLIDE
    float layer_smoothness = config_smoothness[keyboard_state.layer];
    if (!layer_smoothness) {
        layer_smoothness = config_smoothness[0];
    }
    smoothness = layer_smoothness ? layer_smoothness - 1.0f : DEFAULT_TRACKBALL_SMOOTHNESS / 100.0f;

    uint8_t layer_measurement = config_measurement[keyboard_state.layer];
    if (!layer_measurement) {
        layer_measurement = config_measurement[0];
    }
    ratemeter_set_measurement(layer_measurement ? layer_measurement - 1 : DEFAULT_TRACKBALL_MEASUREMENT);
#endif
}

#if VARIANT_TRACKBALL_GLIDE

void trackball_set_smoothness(uint8_t layer, float value)
{
    config_smoothness[layer] = 1.0f + value / 100.0f;
}

void trackball_set_measurement(uint8_t layer, uint8_t value)
{
    config_measurement[layer] = 1 + (value ? TRACKBALL_MEASURE_FAST : TRACKBALL_MEASURE_AVERAGE);
}

/* Live tuning: changes the values that are in use, until the layer is loaded again */
bool trackball_tune_set(uint8_t parameter, uint8_t value)
{
    switch (parameter) {
        case TRACKBALL_TUNE_SPEED:
            acceleration_divisor = 1000.0f / (5.0f * (value ? value : 1));
            return true;
        case TRACKBALL_TUNE_ACCELERATION:
            acceleration_exponent = 1.0f + value / 100.0f;
            return true;
        case TRACKBALL_TUNE_SMOOTHNESS:
            smoothness = 5.0f * value / 100.0f;
            return true;
        case TRACKBALL_TUNE_MEASUREMENT:
            ratemeter_set_measurement(value);
            return true;
        case TRACKBALL_TUNE_RELOAD:
            trackball_load_layer_config();
            return true;
        default:
            return false;
    }
}

bool trackball_tune_get(uint8_t parameter, uint8_t* value)
{
    float v;
    switch (parameter) {
        case TRACKBALL_TUNE_SPEED:
            v = (1000.0f / acceleration_divisor) / 5.0f;
            break;
        case TRACKBALL_TUNE_ACCELERATION:
            v = (acceleration_exponent - 1.0f) * 100.0f;
            break;
        case TRACKBALL_TUNE_SMOOTHNESS:
            v = smoothness * 100.0f / 5.0f;
            break;
        case TRACKBALL_TUNE_MEASUREMENT:
            v = ratemeter_get_measurement();
            break;
        default:
            return false;
    }
    v = roundf(v);
    *value = (v < 0.0f) ? 0 : (v > 255.0f) ? 255 : (uint8_t)v;
    return true;
}

#endif

void trackball_set_acceleration(uint8_t layer, float value)
{
    config_acceleration_exponent[layer] = 1.0f + value;
}

void trackball_set_speed(uint8_t layer, float value)
{
    config_acceleration_divisor[layer] = 1000.0f / value;
}

void trackball_set_scroll_vertical_acceleration(uint8_t layer, float value)
{
    config_scroll_vertical_exponent[layer] = 1.0f + value;
}

void trackball_set_scroll_vertical_speed(uint8_t layer, float value)
{
    config_scroll_vertical_divisor[layer] = 10000.0f / value;
}

void trackball_set_scroll_horizontal_acceleration(uint8_t layer, float value)
{
    config_scroll_horizontal_exponent[layer] = 1.0f + value;
}

void trackball_set_scroll_horizontal_speed(uint8_t layer, float value)
{
    config_scroll_horizontal_divisor[layer] = 10000.0f / value;
}

void trackball_init(void)
{
    // Hall sensors are already configured as EXTI in MX_GPIO_Init
    ratemeter_init(&ratemeter[AXIS_X]);
    ratemeter_init(&ratemeter[AXIS_Y]);
    pointer_buffer[AXIS_X] = 0;
    pointer_buffer[AXIS_Y] = 0;
    distances[AXIS_X] = 0;
    distances[AXIS_Y] = 0;
    wheel_buffer[AXIS_X] = 0;
    wheel_buffer[AXIS_Y] = 0;
    as_wheel = false;
    last_wheel_node = false;
#if VARIANT_TRACKBALL_GLIDE
    glide_reset();
#endif

    trackball_load_layer_config();
}

