#ifndef SIM_TYPES_H
#define SIM_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include "config.h"
#include "rng.h"

typedef struct {
    /* Row-major board. Zero represents the empty cell. */
    uint8_t board[TOTAL_TILES];
    uint8_t empty_index;
    uint32_t move_count;
    uint64_t seed;
    Rng rng;
    bool is_solved;
    bool match_over;
} SimulationState;

typedef struct {
    uint8_t tile_value;
    uint8_t from_index;
    uint8_t to_index;
} MoveResult;

#endif
