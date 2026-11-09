#ifndef DEBUG_H
#define DEBUG_H

#include "app.h"

typedef struct {
    bool enabled;
    float fps;
    float frame_time;
} DebugState;

void debug_init(DebugState *debug);
void debug_update(DebugState *debug, float raw_frame_time);
void debug_draw(const DebugState *debug, const AppView *view, const EventQueue *events);

#endif
