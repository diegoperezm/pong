#ifndef DEBUG_H
#define DEBUG_H

#include "types.h"

void debug_init(
    DebugState *debug
);

void debug_update(
    DebugState *debug,
    const SimulationState *state,
    float frame_time
);

void debug_draw(
    const DebugState *debug,
    const SimulationState *state
);

void debug_draw_collisions(
    const SimulationState *state
);

void debug_draw_velocity(
    const SimulationState *state
);

#endif


