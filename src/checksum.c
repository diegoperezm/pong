#include "checksum.h"
#include "entities.h"
#include "hash.h"

uint64_t sim_checksum(const SimulationState *s) {
    uint64_t h = hash_begin();

    h = hash_u32(h, s->tick);
    h = hash_u32(h, s->match_over ? 1u : 0u);
    h = hash_u32(h, (uint32_t)s->winner);
    h = hash_u32(h, (uint32_t)s->last_scorer);
    h = hash_u32(h, (uint32_t)s->player.score);
    h = hash_u32(h, (uint32_t)s->enemy.score);

    h = hash_u64(h, s->rng.state);
    h = hash_u64(h, s->rng.inc);

    h = hash_f32(h, s->player.x);
    h = hash_f32(h, s->player.y);
    h = hash_f32(h, s->enemy.x);
    h = hash_f32(h, s->enemy.y);

    h = hash_u32(h, s->balls.count);
    h = hash_u32(h, s->balls.next_serial);

    uint32_t order[MAX_BALLS];
    uint32_t n = balls_sorted_by_serial(&s->balls, order);
    for (uint32_t k = 0; k < n; ++k) {
        uint32_t d = order[k];
        h = hash_u32(h, s->balls.serial[d]);
        h = hash_f32(h, s->balls.x[d]);
        h = hash_f32(h, s->balls.y[d]);
        h = hash_f32(h, s->balls.vx[d]);
        h = hash_f32(h, s->balls.vy[d]);
    }

    /* Fixed array order is already canonical for powerups. */
    for (int i = 0; i < MAX_POWERUPS; ++i) {
        if (!s->powerups.active[i]) continue;
        h = hash_u32(h, (uint32_t)i);
        h = hash_f32(h, s->powerups.x[i]);
        h = hash_f32(h, s->powerups.y[i]);
        h = hash_f32(h, s->powerups.lifetime[i]);
        h = hash_u32(h, (uint32_t)s->powerups.type[i]);
    }

    return h;
}
