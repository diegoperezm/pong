#ifndef GAME_H
#define GAME_H

#include "types.h"

void
game_init(
  SimulationState* state
);

void 
game_start(
  SimulationState*state
);

void game_update(
  SimulationState *state,
  GameInput *input,
  float dt
);

// Copy ONLY rendering-relevant simulation data.
void game_make_render_snapshot(
    const SimulationState *state,
    RenderSnapshot *snapshot
);

#endif
