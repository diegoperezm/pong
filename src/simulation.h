#ifndef SIMULATION_H
#define SIMULATION_H

/* The simulation is a closed box.
 *
 *   IN : a seed (sim_begin_match) and one TickInput per tick (sim_step)
 *   OUT: its own state, plus events pushed into the queue it is given
 *
 * It owns no window, no clock, no platform RNG, no menus and no mode. */


#include <stdint.h>
#include "sim_types.h"
#include "events.h"

/* Starts a fresh match: 
 * destroys any previous physics world, wipes ALL state, 
 * seeds the RNG, and initializes a new match. `events` may be NULL. */
void sim_begin_match(SimulationState *state, uint64_t seed, EventQueue *events);

/* Advances exactly one fixed tick. No-op once state->match_over is set.
 * `events` may be NULL. */
void sim_step(SimulationState *state, const TickInput *input, float dt, EventQueue *events);

/* Destroys the physics world. Safe on a never-initialized (zeroed) state. */
void sim_shutdown(SimulationState *state);


#endif



