#include "entities.h"
#include "raylib.h"
#include <math.h>
#include "log.h"


/* ============================================================
 * BALLS
 * ============================================================
 */
void
balls_clear(
    BallPool* balls
)
{
    for (int i = 0; i < MAX_BALLS; ++i) {
        balls->slots[i].active = false;
        // Start generation at 1 so handle generation 0 (INVALID_HANDLE) never matches
        if (balls->slots[i].generation == 0) {
            balls->slots[i].generation = 1;
        }
        balls->x[i]      = 0.0f;
        balls->y[i]      = 0.0f;
        balls->vx[i]     = 0.0f;
        balls->vy[i]     = 0.0f;
        balls->speed[i]  = 0.0f;
    }
}


EntityHandle
ball_create(
    BallPool* balls,
    float     x,
    float     y,
    float     vx,
    float     vy
)
{
    for (uint32_t i = 0; i < MAX_BALLS; ++i) {

        if (balls->slots[i].active)
            continue;

        balls->slots[i].active = true;

        balls->x[i] = x;
        balls->y[i] = y;

        balls->vx[i] = vx;
        balls->vy[i] = vy;

        balls->speed[i] = sqrtf(vx * vx + vy * vy);

        LOG_ENTITY(
          "ball created: slot=%u gen=%u x=%.1f y=%.1f",
          i,
          balls->slots[i].generation,
          x,
          y
        );

        return (EntityHandle){
            .index = i,
            .generation = balls->slots[i].generation
        };
    }

    LOG_ENTITY("ball creation failed: pool full");
    return INVALID_HANDLE;
}


bool 
  ball_is_valid(
    const BallPool* balls,
    EntityHandle handle
)
{
  if(handle.index >= MAX_BALLS) return false;
  const Slot* slot = &balls->slots[handle.index];
  return slot->active && (slot->generation == handle.generation);
}


void
ball_destroy(
    BallPool* balls,
    EntityHandle handle
)
{
   if(!ball_is_valid(balls, handle))  return;

   // incrementing generation invalidates all existing handles 
   balls->slots[handle.index].generation++;
   balls->slots[handle.index].active = false;
}


/* ============================================================
 * PARTICLES
 * ============================================================
 */

void
particles_clear(
    ParticlePool *particles
)
{
    for (int i = 0; i < MAX_PARTICLES; ++i) {

        particles->active[i] = 0;

        particles->x[i] = 0.0f;
        particles->y[i] = 0.0f;

        particles->vx[i] = 0.0f;
        particles->vy[i] = 0.0f;

        particles->lifetime[i] = 0.0f;
        particles->max_lifetime[i] = 0.0f;

        particles->size[i] = 0.0f;
    }
}


void
particles_spawn(
    ParticlePool *particles,
    float x,
    float y,
    int count
)
{
    for (int n = 0; n < count; ++n) {

        int slot = -1;

        /*
         * Find a free particle slot.
         */
        for (int i = 0; i < MAX_PARTICLES; ++i) {
            if (!particles->active[i]) {
                slot = i;
                break;
            }
        }

        /*
         * Pool is full.
         */
        if (slot < 0) return;


        float angle = (float)GetRandomValue(0, 359) * (PI / 180.0f);

        float speed = (float)GetRandomValue(50, 180);


        particles->active[slot] = 1;

        particles->x[slot] = x;
        particles->y[slot] = y;


        particles->vx[slot] = cosf(angle) * speed;

        particles->vy[slot] = sinf(angle) * speed;


        particles->max_lifetime[slot] = 0.25f + (float)GetRandomValue( 0, 100) / 1000.0f; 

	/*
         * IMPORTANT:
         *
         * max_lifetime is an array.
         * We need [slot].
         */
        particles->lifetime[slot] = particles->max_lifetime[slot];
        particles->size[slot] = (float)GetRandomValue( 2, 5);
    }
}


void
particles_update(ParticlePool *particles, float dt)
{
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!particles->active[i])
            continue;

        particles->lifetime[i] -= dt;
        if (particles->lifetime[i] <= 0.0f) {
            particles->active[i] = 0;
            continue;
        }


        particles->x[i] += particles->vx[i] * dt;
        particles->y[i] += particles->vy[i] * dt;
        particles->vx[i] *= 0.98f;
        particles->vy[i] *= 0.98f;
    }
}


/* ============================================================
 * POWERUPS
 * ============================================================
 */

void
powerups_clear(
    PowerupPool *powerups
)
{
    for (int i = 0;
         i < MAX_POWERUPS;
         ++i) {

        powerups->active[i] = 0;

        powerups->x[i] = 0.0f;
        powerups->y[i] = 0.0f;

        powerups->lifetime[i] = 0.0f;

        powerups->type[i] =
            POWERUP_SPEED;
    }
}


int
powerup_create(
    PowerupPool *powerups
)
{
    for (int i = 0;
         i < MAX_POWERUPS;
         ++i) {

        if (powerups->active[i])
            continue;


        powerups->active[i] = 1;


        powerups->x[i] =
            (float)GetRandomValue(
                (int)COURT_LEFT + 80,
                (int)COURT_RIGHT - 80
            );


        powerups->y[i] =
            (float)GetRandomValue(
                (int)COURT_TOP + 40,
                (int)COURT_BOTTOM - 40
            );


        powerups->type[i] =
            GetRandomValue(0, 1) == 0
                ? POWERUP_SPEED
                : POWERUP_MULTI_BALL;


        powerups->lifetime[i] =
            8.0f;


        return i;
    }

    return -1;
}


void
powerups_update(
    PowerupPool *powerups,
    float dt
)
{
    for (int i = 0;
         i < MAX_POWERUPS;
         ++i) {

        if (!powerups->active[i])
            continue;


        powerups->lifetime[i] -= dt;


        if (powerups->lifetime[i] <= 0.0f)
            powerups->active[i] = 0;
    }
}




