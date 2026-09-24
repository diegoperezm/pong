#ifndef ENTITIES_H
#define ENTITIES_H

#include "types.h"

void balls_clear(
  BallPool *balls
);

EntityHandle ball_create(
  BallPool *balls,
  float x,
  float y,
  float direction
);

bool ball_is_valid(
  const BallPool *balls,
  EntityHandle handle
);

void ball_destroy(
  BallPool *balls,
  EntityHandle handle
);

void particles_clear(
  ParticlePool *particles
);

void 
particles_spawn(
  ParticlePool *particles,
  float x,
  float y,
  float vx,
  float vy,
  float lifetime,
  float size
);

void particles_update(
  ParticlePool *particles,
  float dt
);

void powerups_clear(
  PowerupPool *powerups
);

void 
powerup_create(
  PowerupPool *powerups,
  float x,
  float y,
  PowerupType type
);

void powerups_update(
  PowerupPool *powerups,
  float dt
);

#endif



