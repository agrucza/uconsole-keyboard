#include "glider.h"
#include "math_utils.h"
#include "keyboard_variant.h"
#include <math.h>

#if VARIANT_TRACKBALL_GLIDE

void glider_init(Glider* g)
{
    g->direction = 0;
    glider_stop(g);
}

void glider_set_direction(Glider* g, int8_t direction)
{
    if (g->direction != direction) {
        glider_stop(g);
    }
    g->direction = direction;
}

void glider_update(Glider* g, float speed, uint16_t sustain)
{
    g->speed = speed;
    g->sustain = sustain;
    g->release = sustain;
}

void glider_update_speed(Glider* g, float speed)
{
    g->speed = speed;
}

void glider_stop(Glider* g)
{
    g->speed = 0;
    g->sustain = 0;
    g->release = 0;
    g->error = 0;
}

GlideResult glider_glide(Glider* g, uint8_t delta_ms)
{
    const bool already_stopped = (g->speed == 0);

    g->error += g->speed * (float)delta_ms;
    int8_t distance = 0;
    if (g->error > 0) {
        distance = clamp_int8((int32_t)ceilf(g->error));
    }
    g->error -= (float)distance;

    if (g->sustain > 0) {
        g->sustain -= min_uint16(g->sustain, delta_ms);
    } else if (g->release > 0) {
        const uint16_t released = min_uint16(g->release, delta_ms);
        g->speed = g->speed * (float)(g->release - released) / (float)g->release;
        g->release -= released;
    } else {
        g->speed = 0;
    }

    const GlideResult result = { (int8_t)(g->direction * distance), !already_stopped && g->speed == 0 };
    return result;
}

#endif
