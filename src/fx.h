#ifndef FX_H
#define FX_H

/* Visual effects: an EVENT CONSUMER, entirely outside the simulation.
 *
 * Particles used to be spawned inside scoring_system with raylib's RNG.
 * Now the sim only says "a point was scored at (x, y)" and this module
 * decides what that looks like. It has its own RNG stream, so effects can
 * be changed freely without altering a single match, and it advances in
 * lockstep with sim ticks, so replays (and seeks) reproduce the same
 * particles. */

#include <stdint.h>
#include "config.h"
#include "rng.h"
#include "events.h"

typedef struct {
  int   active[MAX_PARTICLES];
  float x[MAX_PARTICLES];
  float y[MAX_PARTICLES];
  float vx[MAX_PARTICLES];
  float vy[MAX_PARTICLES];
  float lifetime[MAX_PARTICLES];
  float max_lifetime[MAX_PARTICLES];
  float size[MAX_PARTICLES];

  int free_head;
  int next_free[MAX_PARTICLES];
} ParticlePool;

typedef struct {
    ParticlePool particles;
    Rng          rng;      /* presentation stream (stream 2) */
    EventCursor  cursor;
} Fx;

/* Clears particles, seeds the RNG from the match seed, and positions the
 * cursor at the current end of `queue`. */
void fx_reset(Fx *fx, uint64_t seed, const EventQueue *queue);

/* Reads every event this consumer has not seen yet. */
void fx_consume_events(Fx *fx, const EventQueue *queue);

/* Advance particles. Called once per sim tick. */
void fx_update(Fx *fx, float dt);

#endif
