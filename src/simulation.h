#ifndef SIMULATION_H
#define SIMULATION_H

#include "types.h"

void simulation_init(SimulationState *state);
void simulation_reset(SimulationState *state);
void simulation_shutdown(SimulationState *state);
void simulation_update(SimulationState *state, const GameInput *input, float dt);

#endif // SIMULATION_H



