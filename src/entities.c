#include "entities.h"
//#include "raylib.h"
#include <math.h>
#include "log.h"

// ============================================================
//  BALLS (Slot Map + SoA Storage)
// ============================================================ 

static b2Vec2 pixel_to_meter(float x, float y) {
    return (b2Vec2){ PX_TO_M(x), PX_TO_M(y) };
}

void ball_pool_init(BallPool* pool) {
    pool->count     = 0; 
    pool->free_head = 0; 

    for (uint32_t i = 0; i < MAX_BALLS; ++i) {
        pool->slots[i].next_free = (i + 1 < MAX_BALLS) ? (i + 1) : INVALID_INDEX;
        pool->slots[i].generation = 1;
        pool->body[i] = b2_nullBodyId;
        pool->x[i]    = 0.0f;
        pool->y[i]    = 0.0f;
        pool->vx[i]   = 0.0f;
        pool->vy[i]   = 0.0f;
        pool->dense_to_sparse[i] = INVALID_INDEX;
    } 
}

void balls_clear(BallPool *balls) {
    for (uint32_t i = 0; i < balls->count; ++i) {
        if (b2Body_IsValid(balls->body[i])) {
            b2DestroyBody(balls->body[i]);
        }
    }
    ball_pool_init(balls);
}

EntityHandle ball_create(BallPool* pool, b2WorldId world, float x, float y, float direction) {
    if (pool->free_head == INVALID_INDEX || pool->count >= MAX_BALLS) {
        LOG_ENTITY("ball creation failed: pool full");
        return INVALID_HANDLE;
    }

    // 1. Pop slot from free list using union's next_free
    uint32_t slot_idx = pool->free_head;
    Slot* slot = &pool->slots[slot_idx];
    pool->free_head = slot->next_free;

    // 2. Map dense index
    uint32_t dense_idx = pool->count++;
    slot->dense_idx = dense_idx;

    float angle = 30.0f * (3.14159265359f / 180.0f);
    float vx    = cosf(angle) * INITIAL_BALL_SPEED * direction;
    float vy    = sinf(angle) * INITIAL_BALL_SPEED;

    // 3. Instantiate Box2D physics body
    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type = b2_dynamicBody;
    body_def.position = pixel_to_meter(x + BALL_SIZE * 0.5f, y + BALL_SIZE * 0.5f);
    body_def.isBullet = true;
    body_def.enableSleep = false;
    
    b2BodyId body = b2CreateBody(world, &body_def);
    b2ShapeDef shape_def = b2DefaultShapeDef();
    shape_def.density = 1.0f;
    shape_def.material.friction = 0.0f;
    shape_def.material.restitution = 1.0f;
    
    b2Circle circle = { .center = {0.0f, 0.0f}, .radius = PX_TO_M(BALL_SIZE * 0.5f) };
    b2CreateCircleShape(body, &shape_def, &circle);
    b2Body_SetLinearVelocity(body, pixel_to_meter(vx, vy));

    // 4. Store component state in dense arrays
    pool->body[dense_idx] = body;
    pool->x[dense_idx]    = x;
    pool->y[dense_idx]    = y;
    pool->vx[dense_idx]   = vx;
    pool->vy[dense_idx]   = vy;
    pool->dense_to_sparse[dense_idx] = slot_idx;

    LOG_ENTITY("ball created: slot=%u gen=%u x=%.1f y=%.1f", slot_idx, slot->generation, x, y);

    return (EntityHandle){
        .index = slot_idx,
        .generation = slot->generation
    };
}

bool ball_is_valid(const BallPool* pool, EntityHandle handle) {
    if (handle.index >= MAX_BALLS || handle.generation == 0) return false;
    return pool->slots[handle.index].generation == handle.generation;
}

void ball_destroy(BallPool* pool, EntityHandle handle) {
    if (!ball_is_valid(pool, handle)) return;

    uint32_t slot_idx = handle.index;
    Slot* slot = &pool->slots[slot_idx];

    uint32_t dead_dense = slot->dense_idx;
    uint32_t last_dense = --pool->count; 

    // 1. Destroy Physics Body
    if (b2Body_IsValid(pool->body[dead_dense])) {
        b2DestroyBody(pool->body[dead_dense]);
    }

    // 2. Swap-and-pop dense storage across all components
    if (dead_dense != last_dense) {
        pool->body[dead_dense] = pool->body[last_dense];
        pool->x[dead_dense]    = pool->x[last_dense];
        pool->y[dead_dense]    = pool->y[last_dense];
        pool->vx[dead_dense]   = pool->vx[last_dense];
        pool->vy[dead_dense]   = pool->vy[last_dense];

        uint32_t moved_slot = pool->dense_to_sparse[last_dense];
        pool->dense_to_sparse[dead_dense] = moved_slot;
        pool->slots[moved_slot].dense_idx = dead_dense;
    }

    // 3. Poison stale array data
    pool->body[last_dense]            = b2_nullBodyId;
    pool->x[last_dense]               = 0.0f;
    pool->y[last_dense]               = 0.0f;
    pool->vx[last_dense]              = 0.0f;
    pool->vy[last_dense]              = 0.0f;
    pool->dense_to_sparse[last_dense] = INVALID_INDEX;

    // 4. Return slot to free list
    slot->generation++;
    slot->next_free = pool->free_head;
    pool->free_head = slot_idx;

    LOG_ENTITY("ball destroyed: slot=%u", slot_idx);
}

// ============================================================
// PARTICLES & POWERUPS
// ============================================================ 
void particles_clear(ParticlePool *particles) {
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        particles->active[i] = 0;
    }
}

void particles_spawn(ParticlePool *particles, float x, float y, float vx, float vy, float lifetime, float size) {
    int slot = -1;
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!particles->active[i]) {
            slot = i;
            break;
        }
    }
    if (slot < 0) return;

    particles->active[slot] = 1;
    particles->x[slot] = x;
    particles->y[slot] = y;
    particles->vx[slot] = vx;
    particles->vy[slot] = vy;
    particles->max_lifetime[slot] = lifetime;
    particles->lifetime[slot] = lifetime;
    particles->size[slot] = size;
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
    }
}

int powerup_create(PowerupPool *powerups, float x, float y, PowerupType type) {
    for (int i = 0; i < MAX_POWERUPS; ++i) {
        if (powerups->active[i]) continue;
        powerups->active[i] = 1;
        powerups->x[i] = x;
        powerups->y[i] = y;
        powerups->type[i] = type;
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


