#ifndef DEBUG_H
#define DEBUG_H

#include "render_types.h"

typedef struct {
  int          enabled;
  int          show_collisions;
  int          show_velocity;
  float        frame_time;
  float        fps;
  int          ball_count;
  int          particle_count;
  int          powerup_count;
} DebugState;

void debug_init(DebugState *debug);

/* raw_frame_time must be the UNCAPPED duration of the last frame
 * (GetFrameTime()), not the value clamped for the simulation.
 * `current` is the snapshot that is being drawn this frame. */
void debug_update(
    DebugState *debug,
    const RenderSnapshot *current,
    float raw_frame_time
);

/* previous/current/alpha must be the same values passed to render_frame()
 * so the overlay is interpolated exactly like the sprites. */
void debug_draw(
    const DebugState *debug,
    const RenderSnapshot *previous,
    const RenderSnapshot *current,
    float alpha
);

#endif
