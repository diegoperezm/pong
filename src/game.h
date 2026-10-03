#ifndef GAME_H
#define GAME_H

#include "sim_types.h"
#include "render_types.h"

void
game_init(
  SimulationState *state
);

void
game_start(
  SimulationState *state
);

/* Call exactly ONCE per rendered frame, right after input_sample().
 *
 * Owns every game-mode transition driven by the player:
 *   TITLE/GAME_OVER --start--> PLAYING
 *   PLAYING         --pause--> PAUSED
 *   PAUSED          --pause--> PLAYING
 *
 * Edge-triggered flags (start, pause) are read only here. They are
 * "consumed" simply because GameInput is re-sampled every frame, so
 * nothing needs to mutate the input. */
void
game_handle_input(
  SimulationState *state,
  const GameInput *input
);

/* Advance the simulation by one FIXED timestep. Call from the
 * accumulator loop, zero or more times per frame.
 *
 * Reads only held-state input (up/down). Does nothing unless the game
 * is in GAME_PLAYING. May itself change the mode (e.g. to GAME_OVER). */
void
game_step(
  SimulationState *state,
  const GameInput *input,
  float dt
);

// Copy ONLY rendering-relevant simulation data.
void
game_make_render_snapshot(
  const SimulationState *state,
  RenderSnapshot *snapshot
);

#endif
