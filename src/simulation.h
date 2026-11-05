#ifndef SIMULATION_H
#define SIMULATION_H

#include <stdint.h>
#include "sim_types.h"
#include "events.h"

void sim_begin_match(SimulationState *state, uint64_t seed, EventQueue *events);
void sim_step(SimulationState *state, const TickInput *input, float dt, EventQueue *events);
void sim_shutdown(SimulationState *state);

#endif
