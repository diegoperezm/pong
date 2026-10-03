#ifndef GAME_TYPES_H
#define GAME_TYPES_H

/* Small vocabulary shared by input, game, sim and render.
 * No physics-engine dependency. */

#include <stdbool.h>
#include "config.h"

typedef struct {
    bool up;
    bool down;
    bool start;
    bool pause;
} GameInput;

typedef enum {
    GAME_TITLE,
    GAME_PLAYING,
    GAME_PAUSED,
    GAME_OVER
} GameMode;

typedef enum {
    POWERUP_SPEED,
    POWERUP_MULTI_BALL,
    POWERUP_FAST_PADDLE
} PowerupType;

#endif
