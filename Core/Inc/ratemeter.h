#ifndef RATEMETER_H
#define RATEMETER_H

#include <stdint.h>
#include "math_utils.h"
#include "keyboard_variant.h"

#define CUTOFF_US 1000000UL

/*
 * How the speed is measured (VARIANT_TRACKBALL_GLIDE only, the other variants
 * always use the running average).
 *
 * TRACKBALL_MEASURE_AVERAGE  running average of the pulse intervals, as in the
 *                            stock firmware. It starts at one pulse per second
 *                            and needs about ten pulses to follow the speed.
 * TRACKBALL_MEASURE_FAST     mean of the last two pulse intervals. The speed is
 *                            known from the second pulse of a stroke on.
 */
#define TRACKBALL_MEASURE_AVERAGE 0
#define TRACKBALL_MEASURE_FAST    1

/* A pulse that comes later than this after the previous one starts a new stroke */
#define STROKE_GAP_US 150000UL
/* Shortest pulse interval that is taken into account, it limits the speed to 500 pulses per second */
#define STROKE_MIN_DELTA_US 2000UL

typedef struct {
    uint64_t lastTime;
    uint32_t averageDelta;
    uint32_t timeout;
#if VARIANT_TRACKBALL_GLIDE
    uint32_t lastDelta;     /* previous pulse interval of this stroke, 0 if there is none */
#endif
} RateMeter;

void ratemeter_set_measurement(uint8_t measurement);
uint8_t ratemeter_get_measurement(void);

void ratemeter_init(RateMeter* rm);
void ratemeter_onInterrupt(RateMeter* rm);
void ratemeter_tick(RateMeter* rm, uint32_t delta);
void ratemeter_expire(RateMeter* rm);
uint16_t ratemeter_delta(const RateMeter* rm);
float ratemeter_rate(const RateMeter* rm);

#endif

