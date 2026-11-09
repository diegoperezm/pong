#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    GAME_TITLE = 0,
    GAME_PLAYING,
    GAME_PAUSED,
    GAME_OVER
} GameMode;

typedef enum {
    DIR_NONE = 0,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} Direction;

typedef struct {
    bool up;
    bool down;
    bool left;
    bool right;
    bool start;
    bool pause;
    bool debug_toggle;
} GameInput;

#endif
