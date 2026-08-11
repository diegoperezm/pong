#ifndef TYPES_H
#define TYPES_H

#include "config.h"

// Game Mode
typedef enum 
{
  GAME_TITLE,
  GAME_PLAYING,
  GAME_PAUSED,
  GAME_OVER 
} GameMode;


/*
   input is transient.
   it is sampled from the platform and consumed
   by the simulation
 */
typedef struct 
{
  int up;
  int down;
  int start;
  int pause;
} GameInput;


// Player/Paddle data
typedef struct
{
  float x;
  float y;
  float width;
  float height;
  float speed;
  int score;
} Paddle;

/*
  Ball pool.
  Each slot represents one ball.
 
  The slot remains at the same index during
  its lifetime.
 */
typedef struct 
{
  int active[MAX_BALLS];
  float x[MAX_BALLS];
  float y[MAX_BALLS];
  float vx[MAX_BALLS];
  float vy[MAX_BALLS];
  float speed[MAX_BALLS];
} BallPool;

// Particle pool

typedef struct 
{
  int active[MAX_PARTICLES];
  int x[MAX_PARTICLES];
  int y[MAX_PARTICLES];
  int vx[MAX_PARTICLES];
  int vy[MAX_PARTICLES];

  float lifetime[MAX_PARTICLES];
  float max_lifetime[MAX_PARTICLES];
  float size[MAX_PARTICLES];

} ParticlePool;

// Power-ups
typedef enum 
{
  POWERUP_SPEED,
  POWERUP_MULTI_BALL,
} PowerupType;

// Power-up pool
typedef struct 
{

  int         active[MAX_POWERUPS];
  float       x[MAX_POWERUPS];
  float       y[MAX_POWERUPS];
  PowerupType type[MAX_POWERUPS];
  float       lifetime[MAX_POWERUPS];

} PowerupPool;


/*
   Complete simulation state
   this is authoritative game state
   Rendering does not modify this
 */

typedef struct 
{
  GameMode     mode;
  float        game_time;
  Paddle       player;
  Paddle       enemy;
  BallPool     balls;
  ParticlePool particles;
  PowerupPool  powerups;
  float        powerup_timer;
  int          winner;
} SimulationState;

/*
   only data required by the renderer
   this is not the complete game state
*/
typedef struct
{
  float player_x;
  float player_y;
  float enemy_x;
  float enemy_y;
  int ball_active[MAX_BALLS];
  float ball_x[MAX_BALLS];
  float ball_y[MAX_BALLS];
 
} RenderSnapshot;

#endif
