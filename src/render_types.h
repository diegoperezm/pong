#ifndef RENDER_TYPES_H
#define RENDER_TYPES_H

/* Plain-data snapshot of the world, safe to hand to the renderer and the
 * debug overlay. No Box2D, no pools, no handles.
 *
 * Balls are matched between two snapshots by SERIAL (never reused), not by
 * slot/generation, so the renderer knows nothing about the sim's storage. */

#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "game_types.h"

typedef struct {
  float        x;
  float        y;
  float        vx;
  float        vy;
  uint32_t     serial;
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
  RenderBall     balls[MAX_BALLS];     /* first ball_count entries are valid */
  int            ball_count;
  RenderParticle particles[MAX_PARTICLES];
  RenderPowerup  powerups[MAX_POWERUPS];
  int            player_score;
  int            enemy_score;
  GameMode       mode;
  int            winner;
  float          game_time;
  uint32_t       tick;
  bool           replaying;
} RenderSnapshot;

#endif
