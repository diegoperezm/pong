#ifndef ENTITIES_H
#define ENTITIES_H
#include "types.h"
#include <stdint.h>
#include <stdbool.h>
//#include "box2d/box2d.h"


// Physics unit scaling helpers (30 pixels = 1 meter)
#define METERS_PER_PIXEL    (1.0f / 30.0f)
#define PIXELS_PER_METER    30.0f

#define PX_TO_M(px) ((px) * METERS_PER_PIXEL)
#define M_TO_PX(m)  ((m) * PIXELS_PER_METER)

// Ball System
void         ball_pool_init(BallPool* pool);
void         balls_clear(BallPool *balls);
EntityHandle ball_create(BallPool* pool, b2WorldId world, float x, float y, float direction);
bool         ball_is_valid(const BallPool* pool, EntityHandle handle);
void         ball_destroy(BallPool* pool, EntityHandle handle);

// Particle System
void particles_clear(ParticlePool *particles);
void particles_spawn(ParticlePool *particles, float x, float y, float vx, float vy, float lifetime, float size);
void particles_update(ParticlePool *particles, float dt);

// Powerup System
void powerups_clear(PowerupPool *powerups);
int  powerup_create(PowerupPool *powerups, float x, float y, PowerupType type);
void powerups_update(PowerupPool *powerups, float dt);

#endif // ENTITIES_H


