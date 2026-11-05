#ifndef SIM_TYPES_H
#define SIM_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "game_types.h"
#include "rng.h"

#define BOARD_SIZE 4
#define SIM_DT (1.0f / 60.0f)

typedef struct {
    uint8_t board[BOARD_SIZE][BOARD_SIZE];
    uint8_t empty_row;
    uint8_t empty_col;
    uint32_t move_count;
    uint32_t tick;
    uint64_t seed;
    Rng rng;
    bool is_solved;
    bool match_over;
} SimulationState;

#endif
