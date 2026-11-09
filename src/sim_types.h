#ifndef SIM_TYPES_H
#define SIM_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "game_types.h"
#include "rng.h"

#define BOARD_SIZE 4
#define TOTAL_TILES (BOARD_SIZE * BOARD_SIZE)
#define SIM_DT (1.0f / 60.0f)
#define TILE_SLIDE_SPEED 18.0f

typedef struct {
    float row;
    float col;
} GridPosition;

typedef struct {
    uint8_t value;      /* 1..15, 0 = empty space */
    GridPosition position;  /* Physical grid position */
    GridPosition target;    /* Target grid position */
    GridPosition velocity;  /* Current velocity */
    bool is_moving;
} TileBody;

typedef struct {
    uint8_t logical_board[BOARD_SIZE][BOARD_SIZE];
    TileBody tiles[TOTAL_TILES - 1]; /* 15 tiles */
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
