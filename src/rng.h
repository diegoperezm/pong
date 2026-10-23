#ifndef RNG_H
#define RNG_H

#include <stdint.h>

typedef struct {
    uint64_t state;
    uint64_t inc;
} Rng;

uint64_t rng_splitmix64(uint64_t state);
void     rng_seed(Rng *rng, uint64_t seed, uint64_t stream);
uint32_t rng_next_u32(Rng *rng);
int      rng_range(Rng *rng, int min, int max);
float    rng_float_range(Rng *rng, float min, float max);

#endif




