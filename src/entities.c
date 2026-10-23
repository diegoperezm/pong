#include "entities.h"
#include <string.h>
#include "log.h"

void entity_pool_init(EntityPool *pool) {
    memset(pool, 0, sizeof(*pool));

    pool->free_head   = 0;
    pool->next_serial = 1;

    for (uint32_t i = 0; i < MAX_ENTITIES; ++i) {
        pool->slots[i].next_free  = (i + 1 < MAX_ENTITIES) ? (i + 1) : INVALID_INDEX;
        pool->slots[i].generation = 1;
        pool->slots[i].active     = false;
        pool->slots[i].dense_idx  = INVALID_INDEX;
        pool->body[i]             = b2_nullBodyId;
        pool->dense_to_sparse[i]  = INVALID_INDEX;
    }
}

void entity_pool_clear(EntityPool *pool) {
    for (uint32_t i = 0; i < pool->count; ++i) {
        if (b2Body_IsValid(pool->body[i])) {
            b2DestroyBody(pool->body[i]);
        }
    }
    entity_pool_init(pool);
}

EntityHandle entity_create(EntityPool *pool, b2WorldId world, float x, float y) {
    if (!b2World_IsValid(world)) return INVALID_HANDLE;

    if (pool->free_head == INVALID_INDEX || pool->count >= MAX_ENTITIES) {
        LOG_ENTITY("entity creation failed: pool full");
        return INVALID_HANDLE;
    }

    uint32_t slot_idx = pool->free_head;
    Slot *slot = &pool->slots[slot_idx];
    pool->free_head = slot->next_free;

    uint32_t dense_idx = pool->count++;
    slot->dense_idx = dense_idx;
    slot->active = true;

    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type = b2_dynamicBody;
    body_def.position = pixel_to_meter(x, y);

    b2BodyId body = b2CreateBody(world, &body_def);
    if (!b2Body_IsValid(body)) {
        pool->count--;
        slot->active = false;
        slot->dense_idx = INVALID_INDEX;
        slot->next_free = pool->free_head;
        pool->free_head = slot_idx;
        return INVALID_HANDLE;
    }

    pool->body[dense_idx] = body;
    pool->x[dense_idx]    = x;
    pool->y[dense_idx]    = y;
    pool->dense_to_sparse[dense_idx] = slot_idx;
    pool->serial[dense_idx] = pool->next_serial++;

    return (EntityHandle){
        .index = slot_idx,
        .generation = slot->generation
    };
}

bool entity_is_valid(const EntityPool *pool, EntityHandle handle) {
    if (handle.index >= MAX_ENTITIES || handle.generation == 0) return false;

    const Slot *slot = &pool->slots[handle.index];
    if (!slot->active || slot->generation != handle.generation) return false;

    uint32_t dense_idx = slot->dense_idx;
    if (dense_idx >= pool->count) return false;

    return pool->dense_to_sparse[dense_idx] == handle.index;
}

void entity_destroy(EntityPool *pool, EntityHandle handle) {
    if (!entity_is_valid(pool, handle)) return;

    uint32_t slot_idx = handle.index;
    Slot *slot = &pool->slots[slot_idx];

    uint32_t dead_dense = slot->dense_idx;
    uint32_t last_dense = --pool->count;

    if (b2Body_IsValid(pool->body[dead_dense])) {
        b2DestroyBody(pool->body[dead_dense]);
    }

    if (dead_dense != last_dense) {
        pool->body[dead_dense]   = pool->body[last_dense];
        pool->x[dead_dense]      = pool->x[last_dense];
        pool->y[dead_dense]      = pool->y[last_dense];
        pool->serial[dead_dense] = pool->serial[last_dense];

        uint32_t moved_slot = pool->dense_to_sparse[last_dense];
        pool->dense_to_sparse[dead_dense] = moved_slot;
        pool->slots[moved_slot].dense_idx = dead_dense;
    }

    pool->body[last_dense]            = b2_nullBodyId;
    pool->x[last_dense]               = 0.0f;
    pool->y[last_dense]               = 0.0f;
    pool->serial[last_dense]          = 0;
    pool->dense_to_sparse[last_dense] = INVALID_INDEX;

    slot->active = false;
    slot->dense_idx = INVALID_INDEX;

    slot->generation++;
    if (slot->generation == 0) {
        slot->generation = 1;
    }

    slot->next_free = pool->free_head;
    pool->free_head = slot_idx;
}

uint32_t entities_sorted_by_serial(const EntityPool *pool, uint32_t order[MAX_ENTITIES]) {
    uint32_t n = pool->count;
    for (uint32_t i = 0; i < n; ++i) {
        uint32_t j = i;
        while (j > 0 && pool->serial[order[j - 1]] > pool->serial[i]) {
            order[j] = order[j - 1];
            --j;
        }
        order[j] = i;
    }
    return n;
}
