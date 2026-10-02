#ifndef DEBUG_H
#define DEBUG_H

#include "types.h"

void debug_init(DebugState *debug);

/* raw_frame_time must be the UNCAPPED duration of the last frame
 * (GetFrameTime()), not the value clamped for the simulation. */
void debug_update(
    DebugState *debug,
    const SimulationState *state,
    float raw_frame_time
);

/* previous/current/alpha must be the same values passed to render_frame()
 * so the overlay is interpolated exactly like the sprites. */
void debug_draw(
    const DebugState *debug,
    const SimulationState *state,
    const RenderSnapshot *previous,
    const RenderSnapshot *current,
    float alpha
);

#endif


