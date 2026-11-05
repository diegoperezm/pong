#ifndef RENDER_TYPES_H
#define RENDER_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "game_types.h"
#include "sim_types.h"

typedef struct {
    uint8_t board[BOARD_SIZE][BOARD_SIZE];
    uint32_t move_count;
    float game_time;
    GameMode mode;
    bool is_solved;
} RenderSnapshot;

#endif
