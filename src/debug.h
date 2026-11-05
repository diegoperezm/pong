#ifndef DEBUG_H
#define DEBUG_H

#include "render_types.h"
#include "events.h"

typedef struct {
    int enabled;
    float fps;
    float frame_time;
} DebugState;

typedef struct {
    const EventQueue *events;
    uint32_t tick;
    uint64_t seed;
    const char *status;
} DebugHistory;

void debug_init(DebugState *debug);
void debug_update(DebugState *debug, const RenderSnapshot *current, float raw_frame_time);
void debug_draw(const DebugState *debug, const RenderSnapshot *previous, const RenderSnapshot *current, float alpha, const DebugHistory *history);

#endif
