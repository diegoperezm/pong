#ifndef ENTITIES_H
#define ENTITIES_H

#include "sim_types.h"
#include <stdint.h>
#include <stdbool.h>
#include "box2d/box2d.h"

#define METERS_PER_PIXEL    (1.0f / 30.0f)
#define PIXELS_PER_METER    30.0f

#define PX_TO_M(px) ((px) * METERS_PER_PIXEL)
#define M_TO_PX(m)  ((m) * PIXELS_PER_METER)

static inline b2Vec2 pixel_to_meter(float x, float y) {
    return (b2Vec2){ PX_TO_M(x), PX_TO_M(y) };
}

/* Generic Entity Pool Template Functions */
void         entity_pool_init(EntityPool *pool);
void         entity_pool_clear(EntityPool *pool);

EntityHandle entity_create(EntityPool *pool, b2WorldId world, float x, float y);
bool         entity_is_valid(const EntityPool *pool, EntityHandle handle);
void         entity_destroy(EntityPool *pool, EntityHandle handle);

/* Returns sorted dense indices for deterministic updates/events */
uint32_t     entities_sorted_by_serial(const EntityPool *pool, uint32_t order[MAX_ENTITIES]);

#endif // ENTITIES_H
