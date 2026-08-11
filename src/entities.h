#ifndef ENTITIES_H
#define ENTITIES_H

#include "types.h"

/*
 * Balls
 */
void balls_clear(BallPool *balls);

int ball_create(
    BallPool *balls,
    float x,
    float y,
    float vx,
    float vy
);

void ball_destroy(
    BallPool *balls,
    int index
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

/*

#ifndef ENTITIES_H
#define ENTITIES_H

#include "types.h"

void balls_clear(BallPool* balls);

int ball_create(
  BallPool* balls,
  float x, 
  float y, 
  float vx, 
  float vy 
);

void ball_destroy(BallPool* balls, int index);
void particles_clear(ParticlePool* particles);

void particles_spawn(
  ParticlePool* particles,
  float x,
  float y, 
  int count
);

void particle_update(ParticlePool* particles, float dt);
void powerups_clear(PowerupPool* powerups);
int powerup_create(PowerupPool* powerups);
void powerups_update(PowerupPool* powerups, float dt);
#endif
*/
