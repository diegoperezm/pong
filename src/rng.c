#include "rng.h"

uint64_t rng_splitmix64(uint64_t x) {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31);
}

void rng_seed(Rng *rng, uint64_t seed, uint64_t stream) {
    uint64_t initial = rng_splitmix64(seed + stream * 0x9e3779b97f4a7c15ULL);
    // Prevent all-zero state for xorshift
    rng->state = (initial == 0ULL) ? 0x123456789ABCDEFULL : initial;
}

uint64_t rng_next_u64(Rng *rng) {
    uint64_t x = rng->state;
    x ^= x >> 12;
    x ^= x << 25;
    x ^= x >> 27;
    rng->state = x;
    return x * 0x2545f4914f6cdd1dULL;
}

uint32_t rng_range(Rng *rng, uint32_t min, uint32_t max) {
    if (min >= max) return min;
    uint64_t span = (uint64_t)max - min + 1ULL;
    return min + (uint32_t)(rng_next_u64(rng) % span);
}
