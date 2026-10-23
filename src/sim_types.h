#ifndef SIM_TYPES_H
#define SIM_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "box2d/box2d.h"
#include "config.h"
#include "rng.h"

typedef enum {
    GAME_TITLE = 0,
    GAME_PLAYING,
    GAME_PAUSED,
    GAME_OVER
} GameMode;

typedef struct {
    bool up;
    bool down;
    bool left;
    bool right;
    bool action1;
    bool action2;
    bool start;
    bool pause;
    bool step;
    bool debug_toggle;
} GameInput;

typedef struct {
    bool action1;
    bool action2;
} TickInput;

typedef struct {
    uint32_t index;
    uint16_t generation;
} EntityHandle;

#define INVALID_INDEX 0xFFFFFFFFu
#define INVALID_HANDLE ((EntityHandle){ .index = INVALID_INDEX, .generation = 0 })

typedef struct {
    uint32_t next_free;
    uint16_t generation;
    bool     active;
    uint32_t dense_idx;
} Slot;

typedef struct {
    Slot         slots[MAX_ENTITIES];
    b2BodyId     body[MAX_ENTITIES];
    float        x[MAX_ENTITIES];
    float        y[MAX_ENTITIES];
    uint32_t     serial[MAX_ENTITIES];
    uint32_t     dense_to_sparse[MAX_ENTITIES];
    uint32_t     free_head;
    uint32_t     next_serial;
    uint32_t     count;
} EntityPool;

typedef struct {
    uint64_t     seed;
    uint32_t     tick;
    bool         match_over;
    int          winner;
    b2WorldId    world;
    EntityPool   entities;
    Rng          rng;
} SimulationState;

#endif // SIM_TYPES_H

