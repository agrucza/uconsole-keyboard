#ifndef TRACKBALL_H
#define TRACKBALL_H

#include <stdint.h>
#include <stdbool.h>
#include "main.h"
#include "ratemeter.h"
#include "usbd_custom_hid_if.h"

#define DEFAULT_TRACKBALL_SPEED 100
#define DEFAULT_TRACKBALL_ACCELERATION 0.2f
#define DEFAULT_TRACKBALL_SCROLL_VERTICAL_SPEED 100
#define DEFAULT_TRACKBALL_SCROLL_VERTICAL_ACCELERATION 0.3f
#define DEFAULT_TRACKBALL_SCROLL_HORIZONTAL_SPEED 100
#define DEFAULT_TRACKBALL_SCROLL_HORIZONTAL_ACCELERATION 0.3f
/* VARIANT_TRACKBALL_GLIDE only */
#define DEFAULT_TRACKBALL_SMOOTHNESS 100
#define DEFAULT_TRACKBALL_MEASUREMENT TRACKBALL_MEASURE_AVERAGE

/* Parameters of the live tuning over USB (VARIANT_TRACKBALL_GLIDE only), see hid_vendor.h */
#define TRACKBALL_TUNE_SPEED        1   /* value * 5 = TRACKBALL_SPEED */
#define TRACKBALL_TUNE_ACCELERATION 2   /* value / 100 = TRACKBALL_ACCELERATION */
#define TRACKBALL_TUNE_SMOOTHNESS   3   /* value * 5 = TRACKBALL_SMOOTHNESS */
#define TRACKBALL_TUNE_MEASUREMENT  4   /* TRACKBALL_MEASURE_AVERAGE or TRACKBALL_MEASURE_FAST */
#define TRACKBALL_TUNE_RELOAD       5   /* write only: back to the values of the layer */

void trackball_init(void);
USBD_StatusTypeDef trackball_task(void);
void trackball_interrupt_x_neg(void);
void trackball_interrupt_x_pos(void);
void trackball_interrupt_y_neg(void);
void trackball_interrupt_y_pos(void);
void trackball_load_layer_config(void);
void trackball_set_acceleration(uint8_t layer, float value);
void trackball_set_speed(uint8_t layer, float value);
void trackball_set_scroll_vertical_acceleration(uint8_t layer, float value);
void trackball_set_scroll_vertical_speed(uint8_t layer, float value);
void trackball_set_scroll_horizontal_acceleration(uint8_t layer, float value);
void trackball_set_scroll_horizontal_speed(uint8_t layer, float value);
void trackball_set_smoothness(uint8_t layer, float value);
void trackball_set_measurement(uint8_t layer, uint8_t value);
bool trackball_tune_set(uint8_t parameter, uint8_t value);
bool trackball_tune_get(uint8_t parameter, uint8_t* value);

#endif

