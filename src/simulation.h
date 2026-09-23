#ifndef SIMULATION_H
#define SIMULATION_H

#include "game.h"
#include <box2d/box2d.h>

void 
simulation_init(
  SimulationState* state
);

void 
simulation_reset(
  SimulationState* state
) ;

void 
simulation_shutdown(
  void
);

void 
simulation_update(
  SimulationState* state,
  const GameInput* input,
  float dt
);

#endif

