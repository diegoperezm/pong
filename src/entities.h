#ifndef ENTITIES_H
#define ENTITIES_H

#include "types.h"

/*
 * Balls
 */
void balls_clear(BallPool *balls);

EntityHandle ball_create(
    BallPool *balls,
    float x,
    float y,
    float vx,
    float vy
);

bool ball_is_valid(
  const BallPool* balls,
  EntityHandle handle
);


void ball_destroy(
    BallPool*    balls,
    EntityHandle index
);


/*
 * Particles
 */
void particles_clear(
    ParticlePool *particles
);

void particles_spawn(
    ParticlePool *particles,
    float x,
    float y,
    int count
);

void particles_update(
    ParticlePool *particles,
    float dt
);


/*
 * Powerups
 */
void powerups_clear(
    PowerupPool *powerups
);

int powerup_create(
    PowerupPool *powerups
);

void powerups_update(
    PowerupPool *powerups,
    float dt
);

#endif


