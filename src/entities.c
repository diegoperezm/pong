#include "entities.h"
#include <math.h>
#include "log.h"

void 
ball_pool_init(BallPool* pool) {
    pool->count     = 0; 
    pool->free_head = 0; 

    for (uint32_t i = 0; i < MAX_BALLS; ++i) {
        pool->slots[i].next_free = (i + 1 < MAX_BALLS) ? (i + 1) : INVALID_INDEX;
        // CORRECCIÓN: 
	// Prevenir el reseteo de la generación de IDs en re-inicios para evitar 
        // validaciones falsas en punteros guardados (ABA problem).
        if (pool->slots[i].generation == 0) {
            pool->slots[i].generation = 1;
        }
	pool->slots[i].active = false;
	pool->slots[i].dense_idx = INVALID_INDEX;
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

EntityHandle 
ball_create(BallPool* pool, b2WorldId world, float x, float y, float direction) {
    if (!b2World_IsValid(world)) return INVALID_HANDLE;

    if (pool->free_head == INVALID_INDEX || pool->count >= MAX_BALLS) {
        LOG_ENTITY("ball creation failed: pool full");
        return INVALID_HANDLE;
    }

    // Allocate slot
    uint32_t slot_idx = pool->free_head;
    Slot* slot = &pool->slots[slot_idx];
    pool->free_head = slot->next_free;

    uint32_t dense_idx = pool->count++;
    slot->dense_idx = dense_idx;
    slot->active = true;

    float angle = 30.0f * (3.14159265359f / 180.0f);
    float vx    = cosf(angle) * INITIAL_BALL_SPEED * direction;
    float vy    = sinf(angle) * INITIAL_BALL_SPEED;

    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type = b2_dynamicBody;
    body_def.position = pixel_to_meter(x + BALL_SIZE * 0.5f, y + BALL_SIZE * 0.5f);
    body_def.isBullet = true;
    body_def.enableSleep = false;
    
    b2BodyId body = b2CreateBody(world, &body_def);
    
    // 1. Rollback if Box2D body creation fails
    if (!b2Body_IsValid(body)) {
        pool->count--; 
        slot->active = false;
        slot->dense_idx = INVALID_INDEX;
        slot->next_free = pool->free_head;
        pool->free_head = slot_idx;
        LOG_ENTITY("ball creation failed: box2d body invalid");
        return INVALID_HANDLE;
    }

    // Body is valid, proceed with shape creation
    b2ShapeDef shape_def = b2DefaultShapeDef();
    shape_def.density = 1.0f;
    shape_def.material.friction = 0.0f;
    shape_def.material.restitution = 1.0f;
    
    b2Circle circle = { .center = {0.0f, 0.0f}, .radius = PX_TO_M(BALL_SIZE * 0.5f) };
    b2ShapeId shape_id = b2CreateCircleShape(body, &shape_def, &circle);

    // 2. Rollback if Box2D shape creation fails (destroy body + reset slot)
    if (!b2Shape_IsValid(shape_id)) {
        b2DestroyBody(body);
        pool->count--; 
        slot->active = false;
        slot->dense_idx = INVALID_INDEX;
        slot->next_free = pool->free_head;
        pool->free_head = slot_idx;
        LOG_ENTITY("ball creation failed: box2d shape invalid");
        return INVALID_HANDLE;
    }

    b2Body_SetLinearVelocity(body, pixel_to_meter(vx, vy));

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

    const Slot *slot = &pool->slots[handle.index];

    // 1. Must be explicitly marked active
    if (!slot->active) return false;

    // 2. Generation must match (guards against ABA reuse)
    if (slot->generation != handle.generation) return false;

    // 3. Bidirectional dense-to-sparse integrity check
    uint32_t dense_idx = slot->dense_idx;
    if (dense_idx >= pool->count) return false;

    return pool->dense_to_sparse[dense_idx] == handle.index;
}

void ball_destroy(BallPool* pool, EntityHandle handle) {
    if (!ball_is_valid(pool, handle)) return;

    uint32_t slot_idx = handle.index;
    Slot* slot = &pool->slots[slot_idx];

    uint32_t dead_dense = slot->dense_idx;
    uint32_t last_dense = --pool->count; 

    if (b2Body_IsValid(pool->body[dead_dense])) {
        b2DestroyBody(pool->body[dead_dense]);
    }

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

    pool->body[last_dense]            = b2_nullBodyId;
    pool->x[last_dense]               = 0.0f;
    pool->y[last_dense]               = 0.0f;
    pool->vx[last_dense]              = 0.0f;
    pool->vy[last_dense]              = 0.0f;
    pool->dense_to_sparse[last_dense] = INVALID_INDEX;

    slot->active = false;
    slot->dense_idx = INVALID_INDEX;
    
    // Safely increment generation, skipping reserved value 0 on wrap-around
    slot->generation++;
    if (slot->generation == 0) {
        slot->generation = 1;
    }

    slot->next_free = pool->free_head;
    pool->free_head = slot_idx;

    LOG_ENTITY("ball destroyed: slot=%u", slot_idx);
}


void particles_clear(ParticlePool *particles) {
    particles->free_head = 0;

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        particles->active[i] = 0;
        // Apuntar al siguiente elemento disponible; -1 indica el final de la lista
        particles->next_free[i] = (i + 1 < MAX_PARTICLES) ? (i + 1) : -1;
    }
}


void particles_spawn(ParticlePool *particles, float x, float y, float vx, float vy, float lifetime, float size) {
    // Si free_head es -1, la reserva de partículas está llena
    if (particles->free_head < 0) return;

    // Extraer ranura libre en O(1)
    int slot = particles->free_head;
    particles->free_head = particles->next_free[slot];

    particles->active[slot]       = 1;
    particles->x[slot]            = x;
    particles->y[slot]            = y;
    particles->vx[slot]           = vx;
    particles->vy[slot]           = vy;
    particles->max_lifetime[slot] = lifetime;
    particles->lifetime[slot]     = lifetime;
    particles->size[slot]         = size;
}


void 
particles_update(ParticlePool *particles, float dt) {
    // Amortiguación independiente de la tasa de refresco (base normalizada a 60 FPS)
    float damping = powf(0.98f, dt * 60.0f);

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!particles->active[i]) continue;

        particles->lifetime[i] -= dt;
        if (particles->lifetime[i] <= 0.0f) {
            particles->active[i] = 0;

            // Devolver la ranura a la lista de libres en O(1)
            particles->next_free[i] = particles->free_head;
            particles->free_head    = i;
            continue;
        }

        particles->x[i]  += particles->vx[i] * dt;
        particles->y[i]  += particles->vy[i] * dt;
        particles->vx[i] *= damping; // Aplicación del factor ajustado a dt
        particles->vy[i] *= damping;
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


