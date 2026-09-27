#ifndef RATEMETER_H
#define RATEMETER_H

#include <stdint.h>
#include "math_utils.h"
#include "keyboard_variant.h"

#define CUTOFF_US 1000000UL

/* A pulse that comes later than this after the previous one starts a new stroke */
#define STROKE_GAP_US 150000UL
/* Shortest pulse interval that is taken into account, it limits the speed to 500 pulses per second */
#define STROKE_MIN_DELTA_US 2000UL

typedef struct {
    uint64_t lastTime;
    uint32_t averageDelta;
    uint32_t timeout;
#if VARIANT_TRACKBALL_SHORT_STROKES
    uint32_t lastDelta;     /* previous pulse interval of this stroke, 0 if there is none */
#endif
} RateMeter;

void ratemeter_init(RateMeter* rm);
void ratemeter_onInterrupt(RateMeter* rm);
void ratemeter_tick(RateMeter* rm, uint32_t delta);
void ratemeter_expire(RateMeter* rm);
uint16_t ratemeter_delta(const RateMeter* rm);
float ratemeter_rate(const RateMeter* rm);

#endif

