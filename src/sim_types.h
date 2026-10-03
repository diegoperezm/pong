#ifndef SIM_TYPES_H
#define SIM_TYPES_H

/* Simulation-side types. This is the ONLY types header that pulls in
 * Box2D. Include it from game, simulation and entities code only. */

#include <stdint.h>
#include <stdbool.h>
#include <box2d/box2d.h>
#include "config.h"
#include "game_types.h"

typedef struct {
    float x;
    float y;
    float width;
    float height;
    float speed;
    int score;
} Paddle;

typedef struct 
{
  uint32_t index;
  uint32_t generation;
} EntityHandle;

static const EntityHandle INVALID_HANDLE = { INVALID_INDEX, 0};

typedef struct {
    uint32_t next_free;
    uint32_t generation;
    uint32_t dense_idx;
    bool     active;      // Explicit live-state tracking
} Slot;

// ============================================================
// BALL POOL (Box2D Stream)
// ============================================================
typedef struct {
    uint32_t count;
    uint32_t free_head;
    Slot     slots[MAX_BALLS];

    b2BodyId body[MAX_BALLS];
    float    x[MAX_BALLS];        
    float    y[MAX_BALLS];       
    float    vx[MAX_BALLS];     
    float    vy[MAX_BALLS];    
    uint32_t dense_to_sparse[MAX_BALLS];
} BallPool;


// ============================================================
// PARTICLES & POWERUPS
// ============================================================
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
  int          active[MAX_POWERUPS];
  float        x[MAX_POWERUPS];
  float        y[MAX_POWERUPS];
  float        lifetime[MAX_POWERUPS];
  PowerupType  type[MAX_POWERUPS];
} PowerupPool;


typedef struct {
    b2WorldId world;     
    b2BodyId player_body;
    b2BodyId enemy_body;

    GameMode mode;
    float game_time;
    int winner;

    Paddle player;
    Paddle enemy;
    
    BallPool balls;
    ParticlePool particles;
    PowerupPool powerups;
    float powerup_timer;
    
    EntityHandle ai_target;
} SimulationState;

#endif
