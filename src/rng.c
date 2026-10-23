#include "rng.h"

uint64_t rng_splitmix64(uint64_t state) {
    state += 0x9e3779b97f4a7c15ULL;
    state = (state ^ (state >> 30)) * 0xbf58476d1ce4e5b9ULL;
    state = (state ^ (state >> 27)) * 0x94d049bb133111ebULL;
    return state ^ (state >> 31);
}

void rng_seed(Rng *rng, uint64_t seed, uint64_t stream) {
    rng->state = 0U;
    rng->inc = (stream << 1U) | 1U;
    rng_next_u32(rng);
    rng->state += seed;
    rng_next_u32(rng);
}

uint32_t rng_next_u32(Rng *rng) {
    uint64_t oldstate = rng->state;
    rng->state = oldstate * 6364136223846793005ULL + rng->inc;
    uint32_t xorshifted = (uint32_t)(((oldstate >> 18U) ^ oldstate) >> 27U);
    uint32_t rot = (uint32_t)(oldstate >> 59U);
    return (xorshifted >> rot) | (xorshifted << ((-rot) & 31));
}

int rng_range(Rng *rng, int min, int max) {
    if (min >= max) return min;
    uint32_t range = (uint32_t)(max - min + 1);
    return min + (int)(rng_next_u32(rng) % range);
}

float rng_float_range(Rng *rng, float min, float max) {
    uint32_t val = rng_next_u32(rng);
    float norm = (float)val / (float)UINT32_MAX;
    return min + norm * (max - min);
}
