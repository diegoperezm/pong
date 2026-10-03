#ifndef SIM_TYPES_H
#define SIM_TYPES_H

/* Simulation-side types. This is the ONLY header (besides entities.h /
 * simulation.h / checksum.h, which include it) that pulls in Box2D. */

#include <stdint.h>
#include <stdbool.h>
#include <box2d/box2d.h>
#include "config.h"
#include "game_types.h"
#include "rng.h"

typedef struct {
    float x;
    float y;
    float width;
    float height;
    float speed;
    int score;
} Paddle;

/* Handles are SIMULATION-PRIVATE: only sim systems hold them, and only
 * within a match. Events, snapshots, replays and the debug view never do.
 * Outside the sim, identity is the ball SERIAL. */
typedef struct {
  uint32_t index;
  uint32_t generation;
} EntityHandle;

static const EntityHandle INVALID_HANDLE = { INVALID_INDEX, 0 };

typedef struct {
    uint32_t next_free;
    uint32_t generation;
    uint32_t dense_idx;
    bool     active;
} Slot;

typedef struct {
    uint32_t count;
    uint32_t free_head;
    uint32_t next_serial;           /* per-match, starts at 1, never reused */
    Slot     slots[MAX_BALLS];

    b2BodyId body[MAX_BALLS];
    float    x[MAX_BALLS];
    float    y[MAX_BALLS];
    float    vx[MAX_BALLS];
    float    vy[MAX_BALLS];
    uint32_t dense_to_sparse[MAX_BALLS];
    uint32_t serial[MAX_BALLS];     /* dense-indexed, moves with swap-remove */
} BallPool;

typedef struct {
  int          active[MAX_POWERUPS];
  float        x[MAX_POWERUPS];
  float        y[MAX_POWERUPS];
  float        lifetime[MAX_POWERUPS];
  PowerupType  type[MAX_POWERUPS];
} PowerupPool;

/* Everything the simulation owns. Particles are NOT here: they are
 * presentation (see fx.h). There is no "game mode" here either: the app
 * owns title/pause/game-over; the sim only knows whether the match is over. */
typedef struct {
    b2WorldId world;
    b2BodyId  player_body;
    b2BodyId  enemy_body;

    uint64_t  seed;
    Rng       rng;                  /* gameplay stream only */
    uint32_t  tick;                 /* number of ticks executed so far */

    bool      match_over;
    int       winner;               /* 0 none, 1 player, 2 enemy */
    int       last_scorer;          /* 0 none, 1 player, 2 enemy */

    Paddle    player;
    Paddle    enemy;

    BallPool    balls;
    PowerupPool powerups;
} SimulationState;

#endif
