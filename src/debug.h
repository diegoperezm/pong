#ifndef DEBUG_H
#define DEBUG_H

#include <stdbool.h>
#include <stdint.h>
#include "render_types.h"
#include "events.h"

typedef struct {
  int          enabled;
  int          show_collisions;
  int          show_velocity;
  float        frame_time;
  float        fps;
  int          ball_count;
  int          particle_count;
  int          powerup_count;

  // Raygui interactive controls state
  float        spawn_ball_speed;
  bool         trigger_spawn_ball;
  bool         clear_balls_trigger;

} DebugState;

/* Read-only view of the history the app keeps. Nothing here is simulation
 * state: it is the event ring, the input log, and replay bookkeeping. */
typedef struct {
    const EventQueue *events;
    uint32_t          tick;
    uint64_t          seed;
    uint32_t          desync_tick;
    uint32_t          events_lost;
    const char       *status;
} DebugHistory;

void debug_init(DebugState *debug);

/* raw_frame_time must be the UNCAPPED duration of the last frame. */
void debug_update(DebugState *debug, const RenderSnapshot *current, float raw_frame_time);

void debug_draw(
    const DebugState *debug,
    const RenderSnapshot *previous,
    const RenderSnapshot *current,
    float alpha,
    const DebugHistory *history
);

#endif
