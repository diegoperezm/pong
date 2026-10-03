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

/* Starts a fresh match: destroys any previous Box2D world, wipes ALL state
 * (so nothing leaks between matches), seeds the RNG, builds a new world and
 * serves the first ball. `events` may be NULL. */
void sim_begin_match(SimulationState *state, uint64_t seed, EventQueue *events);

/* Advances exactly one fixed tick. No-op once state->match_over is set.
 * `events` may be NULL. */
void sim_step(SimulationState *state, const TickInput *input, float dt, EventQueue *events);

/* Destroys the Box2D world. Safe on a never-initialized (zeroed) state. */
void sim_shutdown(SimulationState *state);

/* Hash of every constant that changes how a match plays out, plus a manual
 * logic-version number. Stored in replays; a mismatch means old replays
 * would not reproduce, so they are refused. */
uint64_t sim_config_hash(void);

#endif // SIMULATION_H
