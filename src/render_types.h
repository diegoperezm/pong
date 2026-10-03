#ifndef RENDER_TYPES_H
#define RENDER_TYPES_H

/* Plain-data snapshot of the simulation, safe to hand to the renderer
 * and the debug overlay. No Box2D, no pools, no SimulationState. */

#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "game_types.h"

typedef struct {
  bool         active;
  float        x;
  float        y;
  float        vx;
  float        vy;
  uint32_t     generation;
} RenderBall;

typedef struct {
  bool           active;
  float          x;
  float          y;
  float          lifetime;
  float          max_lifetime;
  float          size;
} RenderParticle;

typedef struct {
  bool           active;
  float          x;
  float          y;
  PowerupType    type;
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
  float          game_time;     /* added so debug_draw needs no SimulationState */
} RenderSnapshot;

#endif
