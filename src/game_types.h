#ifndef GAME_TYPES_H
#define GAME_TYPES_H

#include <stdbool.h>

typedef enum {
    GAME_TITLE,
    GAME_PLAYING,
    GAME_PAUSED,
    GAME_OVER
} GameMode;

typedef enum {
    SIDE_NONE = 0,
    SIDE_PLAYER,
    SIDE_ENEMY
} Side;

typedef enum {
    WALL_NONE = 0,
    WALL_TOP,
    WALL_BOTTOM
} Wall;

typedef struct {
    bool up;
    bool down;
    bool left;
    bool right;
    bool start;
    bool pause;
    bool debug_toggle;
} GameInput;

typedef struct {
    bool up;
    bool down;
    bool left;
    bool right;
} TickInput;

#endif
