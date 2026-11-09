#ifndef SIMULATION_H
#define SIMULATION_H

#include <stdbool.h>
#include <stdint.h>
#include "events.h"
#include "game_types.h"
#include "sim_types.h"

void sim_begin_match(SimulationState *state, uint64_t seed, EventQueue *events);
bool sim_can_move(const SimulationState *state, Direction direction);
bool sim_move(SimulationState *state, Direction direction, MoveResult *result, EventQueue *events);
bool sim_check_solved(const SimulationState *state);
void sim_shutdown(SimulationState *state);

#endif
