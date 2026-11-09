#ifndef RNG_H
#define RNG_H

#include <stdint.h>

typedef struct {
    uint64_t state;
} Rng;

void rng_seed(Rng *rng, uint64_t seed, uint64_t stream);
uint64_t rng_next_u64(Rng *rng);
uint32_t rng_range(Rng *rng, uint32_t min, uint32_t max);
uint64_t rng_splitmix64(uint64_t x);

#endif
