#ifndef SIMULATION_H
#define SIMULATION_H

#include "types.h"

void simulation_update(
    SimulationState *state,
    const GameInput *input,
    float dt
);
#endif

