#ifndef GLIDER_H
#define GLIDER_H

#include <stdint.h>
#include <stdbool.h>

/*
 * Turns the pulses of the trackball into smooth pointer movement.
 *
 * Every pulse sets a speed. The pointer keeps this speed for the sustain time,
 * then the speed falls to zero during the release time, which is as long as
 * the sustain time. A new pulse starts over. This is the algorithm of the stock
 * DevTerm firmware.
 */
typedef struct {
    int8_t direction;   /* +1 or -1 */
    float speed;        /* pixels per millisecond */
    uint16_t sustain;   /* remaining time at full speed, ms */
    uint16_t release;   /* remaining time of the slow down, ms */
    float error;        /* distance that is not sent yet, pixels */
} Glider;

typedef struct {
    int8_t value;       /* pixels to move now */
    bool stopped;       /* the movement has ended with this step */
} GlideResult;

void glider_init(Glider* g);
void glider_set_direction(Glider* g, int8_t direction);
void glider_update(Glider* g, float speed, uint16_t sustain);
void glider_update_speed(Glider* g, float speed);
void glider_stop(Glider* g);
GlideResult glider_glide(Glider* g, uint8_t delta_ms);

#endif
