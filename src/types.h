#ifndef TYPES_H
#define TYPES_H

#include "config.h"
#include <stdint.h>
#include <stdbool.h>
#include <box2d/box2d.h>

typedef struct {
    bool up;
    bool down;
    bool start;
} GameInput;

typedef enum { 
 GAME_TITLE,
 GAME_PLAYING,
 GAME_PAUSED,
 GAME_OVER 
} 
GameMode;

typedef struct {
    float x, y, width, height, speed;
    int score;
} Paddle;

typedef struct 
{
  uint32_t index;
  uint32_t generation;
} EntityHandle;

static const EntityHandle INVALID_HANDLE = { INVALID_INDEX, 0};

typedef struct {
    union {
        uint32_t dense_idx; // Used when the entity is alive
        uint32_t next_free; // Used when the entity is dead
    };
    uint32_t generation;
} Slot;




// ============================================================
// BALL POOL (Box2D Stream)
// ============================================================
typedef struct {
    uint32_t count;
    uint32_t free_head;
    Slot slots[MAX_BALLS];

    // Parallel dense arrays (SoA layout)
    b2BodyId body[MAX_BALLS];
    float x[MAX_BALLS];        // <-- Make sure this line exists
    float y[MAX_BALLS];        // <-- Make sure this line exists
    float vx[MAX_BALLS];       // <-- Make sure this line exists
    float vy[MAX_BALLS];       // <-- Make sure this line exists
    uint32_t dense_to_sparse[MAX_BALLS];
} BallPool;


// ============================================================
// PARTICLES & POWERUPS
// ============================================================
typedef enum { 
  POWERUP_SPEED,
  POWERUP_MULTI_BALL 
} PowerupType;

typedef struct {
  int   active[MAX_PARTICLES];
  float x[MAX_PARTICLES];
  float y[MAX_PARTICLES];
  float vx[MAX_PARTICLES];
  float vy[MAX_PARTICLES];
  float lifetime[MAX_PARTICLES];
  float max_lifetime[MAX_PARTICLES];
  float size[MAX_PARTICLES];
} ParticlePool;

typedef struct {
  int          active[MAX_POWERUPS];
  float        x[MAX_POWERUPS];
  float        y[MAX_POWERUPS];
  float        lifetime[MAX_POWERUPS];
  PowerupType  type[MAX_POWERUPS];
} PowerupPool;


typedef struct {
  int   enabled;
  int   show_collisions;
  int   show_velocity;
  float frame_time;
  float fps;
  int   ball_count;
  int   particle_count;
  int   powerup_count;
} DebugState;


typedef struct {
  bool active;
  float x;
  float y;
} RenderBall;

typedef struct {
  bool active;
  float x;
  float y;
  float lifetime;
  float max_lifetime;
  float size;
} RenderParticle;

typedef struct {
  bool active;
  float x;
  float y;
  PowerupType type;
} RenderPowerup;

typedef struct {
  float          player_x;
  float          player_y;
  float          enemy_x;
  float          enemy_y;
  RenderBall     balls[MAX_BALLS];
  int            ball_count;
  RenderParticle particles[MAX_PARTICLES];
  RenderPowerup  powerups[MAX_POWERUPS];
  int            player_score;
  int            enemy_score;
  GameMode       mode;
  int            winner;
} RenderSnapshot;

typedef struct {
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

    b2WorldId world; // Shared world state
} SimulationState;



#endif



