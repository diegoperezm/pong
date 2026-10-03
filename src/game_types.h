#ifndef GAME_TYPES_H
#define GAME_TYPES_H

/* Small vocabulary shared by input, app, sim and render.
 * No physics-engine dependency. */

#include <stdbool.h>
#include "config.h"

#ifndef COURT_CENTER_X
#define COURT_CENTER_X ((COURT_LEFT + COURT_RIGHT) * 0.5f)
#endif
#ifndef COURT_CENTER_Y
#define COURT_CENTER_Y ((COURT_TOP + COURT_BOTTOM) * 0.5f)
#endif

/* APP-level input: sampled once per rendered frame. The simulation never
 * sees this struct. Menus, pause, replay and debug controls live here. */
typedef struct {
    bool up;            /* held */
    bool down;          /* held */
    bool start;         /* edge */
    bool pause;         /* edge */
    bool replay;        /* edge: watch the last recorded match */
    bool step;          /* edge: advance one tick while paused in a replay */
    bool seek_back;     /* edge: replay, jump back one second */
    bool seek_fwd;      /* edge: replay, jump forward one second */
    bool debug_toggle;  /* edge */
} GameInput;

/* The ONLY input the simulation consumes: exactly one record per tick. */
typedef struct {
    bool up;
    bool down;
} TickInput;

static inline bool tick_input_equal(TickInput a, TickInput b) {
    return a.up == b.up && a.down == b.down;
}

/* App-level mode (the simulation has no mode of its own). */
typedef enum {
    GAME_TITLE,
    GAME_PLAYING,
    GAME_PAUSED,
    GAME_OVER
} GameMode;

typedef enum { SIDE_PLAYER = 0, SIDE_ENEMY = 1 } Side;
typedef enum { WALL_TOP = 0, WALL_BOTTOM = 1 } Wall;

typedef enum {
    POWERUP_SPEED,
    POWERUP_MULTI_BALL,
    POWERUP_FAST_PADDLE
} PowerupType;

#endif
