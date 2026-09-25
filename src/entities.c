#include "entities.h"
#include "raylib.h"
#include <math.h>
#include "log.h"

// ============================================================
//  BALLS (Slot Map Implementation)
// ============================================================ 

void ball_pool_init(BallPool* pool) {
    pool->count     = 0; 
    pool->free_head = 0; 

    // Link all slots into a free-list chain 
    for (uint32_t i = 0; i < MAX_BALLS; ++i) {
        pool->slots[i].dense_idx = (i + 1 < MAX_BALLS) ? (i + 1) : INVALID_INDEX;
        // Generation 0 is reserved for INVALID_HANDLE
        pool->slots[i].generation = 1;
    } 
}

void balls_clear(BallPool *balls) {
    ball_pool_init(balls);
}

EntityHandle ball_create(BallPool* pool, float x, float y, float direction) {
    // 1. Check if pool is full
    if (pool->free_head == INVALID_INDEX || pool->count >= MAX_BALLS) {
        LOG_ENTITY("ball creation failed: pool full");
        return INVALID_HANDLE;
    }

    // 2. Pop slot from free list
    uint32_t slot_idx = pool->free_head;
    Slot* slot = &pool->slots[slot_idx];

    // Advance free head to next available slot
    pool->free_head = slot->dense_idx;

    // 3. Append data to end of dense array
    uint32_t dense_idx = pool->count++;
    slot->dense_idx = dense_idx;

    float angle = 30.0f * (3.14159265359f / 180.0f);
    float vx    = cosf(angle) * INITIAL_BALL_SPEED * direction;
    float vy    = sinf(angle) * INITIAL_BALL_SPEED;

    pool->x[dense_idx]                = x;
    pool->y[dense_idx]                = y;
    pool->vx[dense_idx]               = vx;
    pool->vy[dense_idx]               = vy;
    pool->speed[dense_idx]            = INITIAL_BALL_SPEED;
    pool->dense_to_sparse[dense_idx]  = slot_idx;

    LOG_ENTITY("ball created: slot=%u gen=%u x=%.1f y=%.1f", slot_idx, slot->generation, x, y);

    return (EntityHandle){
        .index = slot_idx,
        .generation = slot->generation
    };
}

bool ball_is_valid(const BallPool* pool, EntityHandle handle) {
    if (handle.index >= MAX_BALLS || handle.generation == 0)
        return false;

    return pool->slots[handle.index].generation == handle.generation;
}

void ball_destroy(BallPool* pool, EntityHandle handle) {
    if (!ball_is_valid(pool, handle))
        return;

    uint32_t slot_idx = handle.index;
    Slot* slot = &pool->slots[slot_idx];

    uint32_t dead_dense = slot->dense_idx;
    uint32_t last_dense = --pool->count; 

    // 1. Swap-and-pop dense storage
    if (dead_dense != last_dense) {
        // Move last active element into dead element's position
        pool->x[dead_dense]     = pool->x[last_dense];
        pool->y[dead_dense]     = pool->y[last_dense];
        pool->vx[dead_dense]    = pool->vx[last_dense];
        pool->vy[dead_dense]    = pool->vy[last_dense];
        pool->speed[dead_dense] = pool->speed[last_dense];

        // Repair back-pointer & sparse mapping for moved element
        uint32_t moved_slot = pool->dense_to_sparse[last_dense];
        pool->dense_to_sparse[dead_dense] = moved_slot;
        pool->slots[moved_slot].dense_idx = dead_dense;
    }

    // 2. Invalidate handle & Push slot back onto free list
    slot->generation++;
    slot->dense_idx = pool->free_head;
    pool->free_head = slot_idx;

    LOG_ENTITY("ball destroyed: slot=%u", slot_idx);
}

// ============================================================
// PARTICLES & POWERUPS
// ============================================================ 

void particles_clear(ParticlePool *particles) {
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

void particles_spawn(ParticlePool *particles, float x, float y, float vx, float vy, float lifetime, float size) {
    (void)vx; (void)vy; (void)lifetime; (void)size;
    for (int n = 0; n < 20; ++n) {
        int slot = -1;
        for (int i = 0; i < MAX_PARTICLES; ++i) {
            if (!particles->active[i]) {
                slot = i;
                break;
            }
        }
        if (slot < 0) return;

        float angle = (float)GetRandomValue(0, 359) * (PI / 180.0f);
        float speed = (float)GetRandomValue(50, 180);

        particles->active[slot] = 1;
        particles->x[slot] = x;
        particles->y[slot] = y;
        particles->vx[slot] = cosf(angle) * speed;
        particles->vy[slot] = sinf(angle) * speed;
        particles->max_lifetime[slot] = 0.25f + (float)GetRandomValue(0, 100) / 1000.0f;
        particles->lifetime[slot] = particles->max_lifetime[slot];
        particles->size[slot] = (float)GetRandomValue(2, 5);
    }
}

void particles_update(ParticlePool *particles, float dt) {
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!particles->active[i]) continue;
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

void powerups_clear(PowerupPool *powerups) {
    for (int i = 0; i < MAX_POWERUPS; ++i) {
        powerups->active[i] = 0;
        powerups->x[i] = 0.0f;
        powerups->y[i] = 0.0f;
        powerups->lifetime[i] = 0.0f;
        powerups->type[i] = POWERUP_SPEED;
    }
}

int powerup_create(PowerupPool *powerups, float x, float y, PowerupType type) {
    (void)x; (void)y; (void)type;
    for (int i = 0; i < MAX_POWERUPS; ++i) {
        if (powerups->active[i]) continue;
        powerups->active[i] = 1;
        powerups->x[i] = (float)GetRandomValue((int)COURT_LEFT + 80, (int)COURT_RIGHT - 80);
        powerups->y[i] = (float)GetRandomValue((int)COURT_TOP + 40, (int)COURT_BOTTOM - 40);
        powerups->type[i] = GetRandomValue(0, 1) == 0 ? POWERUP_SPEED : POWERUP_MULTI_BALL;
        powerups->lifetime[i] = 8.0f;
        return i;
    }
    return -1;
}



void powerups_update(PowerupPool *powerups, float dt) {
    for (int i = 0; i < MAX_POWERUPS; ++i) {
        if (!powerups->active[i]) continue;
        powerups->lifetime[i] -= dt;
        if (powerups->lifetime[i] <= 0.0f)
            powerups->active[i] = 0;
    }
}

