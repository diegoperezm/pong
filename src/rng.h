#ifndef RNG_H
#define RNG_H

/* PCG32. Small, fast, and fully reproducible: the whole generator state is
 * two integers, so it can live inside SimulationState and be hashed.
 *
 * Streams: the simulation uses stream 1 (gameplay randomness only).
 * Presentation code (particles) uses its own generator on stream 2, so
 * tweaking a visual effect can never change how a match plays out. */

#include <stdint.h>

typedef struct {
    uint64_t state;
    uint64_t inc;
} Rng;

static inline uint32_t rng_next_u32(Rng *r) {
    uint64_t old = r->state;
    r->state = old * 6364136223846793005ULL + r->inc;
    uint32_t xorshifted = (uint32_t)(((old >> 18u) ^ old) >> 27u);
    uint32_t rot = (uint32_t)(old >> 59u);
    return (xorshifted >> rot) | (xorshifted << ((32u - rot) & 31u));
}

static inline void rng_seed(Rng *r, uint64_t seed, uint64_t stream) {
    r->state = 0u;
    r->inc   = (stream << 1u) | 1u;
    (void)rng_next_u32(r);
    r->state += seed;
    (void)rng_next_u32(r);
}

/* Uniform integer in [lo, hi], inclusive. Requires lo <= hi. */
static inline int rng_range(Rng *r, int lo, int hi) {
    uint32_t span = (uint32_t)(hi - lo) + 1u;
    return lo + (int)(((uint64_t)rng_next_u32(r) * span) >> 32);
}

/* Uniform float in [0, 1). */
static inline float rng_float01(Rng *r) {
    return (float)(rng_next_u32(r) >> 8) * (1.0f / 16777216.0f);
}

static inline float rng_float_range(Rng *r, float lo, float hi) {
    return lo + (hi - lo) * rng_float01(r);
}

/* Used OUTSIDE the simulation to turn "base seed + match number" into a
 * well-mixed per-match seed. */
static inline uint64_t rng_splitmix64(uint64_t x) {
    x += 0x9E3779B97F4A7C15ULL;
    x = (x ^ (x >> 30)) * 0xBF58476D1CE4E5B9ULL;
    x = (x ^ (x >> 27)) * 0x94D049BB133111EBULL;
    return x ^ (x >> 31);
}

#endif
