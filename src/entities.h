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

static inline
b2Vec2 pixel_to_meter(float x, float y) {
  return (b2Vec2){ PX_TO_M(x), PX_TO_M(y) };
}

/* ---- Ball pool --------------------------------------------------------
 * ball_pool_init() is a FULL reset: generations and the serial counter
 * restart. That is safe because handles never outlive a match. */
void         ball_pool_init(BallPool *pool);
void         balls_clear(BallPool *balls);

/* x, y: top-left in px. vx, vy: px/s. */
EntityHandle ball_create(BallPool *pool, b2WorldId world,
                         float x, float y, float vx, float vy);
bool         ball_is_valid(const BallPool *pool, EntityHandle handle);
void         ball_destroy(BallPool *pool, EntityHandle handle);

/* Fills `order` with dense indices sorted by ascending serial and returns
 * the count. This is the canonical iteration order for anything that emits
 * events or feeds a checksum, so results never depend on pool layout. */
uint32_t     balls_sorted_by_serial(const BallPool *pool, uint32_t order[MAX_BALLS]);

/* ---- Powerups --------------------------------------------------------- */
void powerups_clear(PowerupPool *powerups);
int  powerup_create(PowerupPool *powerups, float x, float y, PowerupType type);
void powerups_update(PowerupPool *powerups, float dt);

#endif // ENTITIES_H
