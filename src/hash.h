#ifndef HASH_H
#define HASH_H

/* 64-bit FNV-1a, fed byte-by-byte in a fixed (little-endian) order so the
 * result does not depend on host endianness. Used for state checksums and
 * the replay config hash. */

#include <stddef.h>
#include <stdint.h>
#include <string.h>

#define HASH_FNV_OFFSET 14695981039346656037ULL
#define HASH_FNV_PRIME  1099511628211ULL

static inline uint64_t hash_begin(void) { return HASH_FNV_OFFSET; }

static inline uint64_t hash_byte(uint64_t h, uint8_t b) {
    return (h ^ b) * HASH_FNV_PRIME;
}

static inline uint64_t hash_u32(uint64_t h, uint32_t v) {
    h = hash_byte(h, (uint8_t)(v));
    h = hash_byte(h, (uint8_t)(v >> 8));
    h = hash_byte(h, (uint8_t)(v >> 16));
    h = hash_byte(h, (uint8_t)(v >> 24));
    return h;
}

static inline uint64_t hash_u64(uint64_t h, uint64_t v) {
    h = hash_u32(h, (uint32_t)(v));
    return hash_u32(h, (uint32_t)(v >> 32));
}

/* Hash the bit pattern of a float. -0.0 is folded into +0.0 so two states
 * that are numerically equal never hash differently. */
static inline uint64_t hash_f32(uint64_t h, float f) {
    uint32_t bits;
    if (f == 0.0f) f = 0.0f;
    memcpy(&bits, &f, sizeof bits);
    return hash_u32(h, bits);
}

#endif
