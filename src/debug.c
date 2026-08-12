#include "debug.h"

#include "raylib.h"

#include <stdio.h>


/* ============================================================
 * CONFIGURATION
 * ============================================================
 */

#define DEBUG_FONT_SIZE       16
#define DEBUG_LINE_HEIGHT     20

#define DEBUG_PANEL_X         10
#define DEBUG_PANEL_Y         10

#define DEBUG_PANEL_WIDTH     300
#define DEBUG_PANEL_HEIGHT    350

#define DEBUG_PADDING         10


/* ============================================================
 * INITIALIZATION
 * ============================================================
 */

void
debug_init(
    DebugState *debug
)
{
    debug->enabled = 0;
    debug->show_collisions = 0;
    debug->show_velocity = 0;

    debug->frame_time = 0.0f;
    debug->fps = 0.0f;

    debug->ball_count = 0;
    debug->particle_count = 0;
    debug->powerup_count = 0;
}




/* ============================================================
 * ENTITY COUNTERS
 * ============================================================
 */

static int
debug_count_balls(
    const SimulationState *state
)
{
    int count = 0;

    for (int i = 0;
         i < MAX_BALLS;
         ++i) {

        if (state->balls.active[i])
            ++count;
    }

    return count;
}


static int
debug_count_particles(
    const SimulationState *state
)
{
    int count = 0;

    for (int i = 0;
         i < MAX_PARTICLES;
         ++i) {

        if (state->particles.active[i])
            ++count;
    }

    return count;
}


static int
debug_count_powerups(
    const SimulationState *state
)
{
    int count = 0;

    for (int i = 0;
         i < MAX_POWERUPS;
         ++i) {

        if (state->powerups.active[i])
            ++count;
    }

    return count;
}


/* ============================================================
 * UPDATE
 * ============================================================
 */

void
debug_update(
    DebugState *debug,
    const SimulationState *state,
    float frame_time
)
{
    if (debug == NULL ||
        state == NULL) {

        return;
    }


    debug->frame_time =
        frame_time;


    if (frame_time > 0.0f) {

        debug->fps =
            1.0f / frame_time;

    } else {

        debug->fps = 0.0f;
    }


    debug->ball_count =
        debug_count_balls(state);

    debug->particle_count =
        debug_count_particles(state);

    debug->powerup_count =
        debug_count_powerups(state);
}


/* ============================================================
 * TEXT
 * ============================================================
 */

static void
debug_text(
    int x,
    int y,
    const char *text
)
{
    DrawText(
        text,
        x,
        y,
        DEBUG_FONT_SIZE,
        RAYWHITE
    );
}


/* ============================================================
 * GAME MODE
 * ============================================================
 */

static const char *
debug_game_mode_name(
    GameMode mode
)
{
    switch (mode) {

        case GAME_TITLE:
            return "TITLE";

        case GAME_PLAYING:
            return "PLAYING";

        case GAME_PAUSED:
            return "PAUSED";

        case GAME_OVER:
            return "GAME OVER";

        default:
            return "UNKNOWN";
    }
}


/* ============================================================
 * MAIN DEBUG OVERLAY
 * ============================================================
 */

void
debug_draw(
    const DebugState *debug,
    const SimulationState *state
)
{
    if (debug == NULL ||
        state == NULL) {

        return;
    }


    if (!debug->enabled)
        return;


    /*
     * Panel background.
     */

    DrawRectangle(
        DEBUG_PANEL_X,
        DEBUG_PANEL_Y,
        DEBUG_PANEL_WIDTH,
        DEBUG_PANEL_HEIGHT,
        Fade(BLACK, 0.80f)
    );


    /*
     * Panel border.
     */

    DrawRectangleLines(
        DEBUG_PANEL_X,
        DEBUG_PANEL_Y,
        DEBUG_PANEL_WIDTH,
        DEBUG_PANEL_HEIGHT,
        RAYWHITE
    );


    int x =
        DEBUG_PANEL_X +
        DEBUG_PADDING;

    int y =
        DEBUG_PANEL_Y +
        DEBUG_PADDING;


    char text[128];


    /* --------------------------------------------------------
     * Header
     * --------------------------------------------------------
     */

    debug_text(
        x,
        y,
        "DEBUG"
    );

    y += DEBUG_LINE_HEIGHT + 4;


    /* --------------------------------------------------------
     * Frame
     * --------------------------------------------------------
     */

    snprintf(
        text,
        sizeof(text),
        "FPS: %.1f",
        debug->fps
    );

    debug_text(
        x,
        y,
        text
    );

    y += DEBUG_LINE_HEIGHT;


    snprintf(
        text,
        sizeof(text),
        "Frame: %.3f ms",
        debug->frame_time * 1000.0f
    );

    debug_text(
        x,
        y,
        text
    );

    y += DEBUG_LINE_HEIGHT;


    /* --------------------------------------------------------
     * Game
     * --------------------------------------------------------
     */

    debug_text(
        x,
        y,
        "GAME"
    );

    y += DEBUG_LINE_HEIGHT;


    snprintf(
        text,
        sizeof(text),
        "  Mode: %s",
        debug_game_mode_name(
            state->mode
        )
    );

    debug_text(
        x,
        y,
        text
    );

    y += DEBUG_LINE_HEIGHT;


    snprintf(
        text,
        sizeof(text),
        "  Time: %.2f",
        state->game_time
    );

    debug_text(
        x,
        y,
        text
    );

    y += DEBUG_LINE_HEIGHT;


    /* --------------------------------------------------------
     * Player
     * --------------------------------------------------------
     */

    debug_text(
        x,
        y,
        "PLAYER"
    );

    y += DEBUG_LINE_HEIGHT;


    snprintf(
        text,
        sizeof(text),
        "  X: %.1f",
        state->player.x
    );

    debug_text(
        x,
        y,
        text
    );

    y += DEBUG_LINE_HEIGHT;


    snprintf(
        text,
        sizeof(text),
        "  Y: %.1f",
        state->player.y
    );

    debug_text(
        x,
        y,
        text
    );

    y += DEBUG_LINE_HEIGHT;


    snprintf(
        text,
        sizeof(text),
        "  Score: %d",
        state->player.score
    );

    debug_text(
        x,
        y,
        text
    );

    y += DEBUG_LINE_HEIGHT;


    /* --------------------------------------------------------
     * Enemy
     * --------------------------------------------------------
     */

    debug_text(
        x,
        y,
        "ENEMY"
    );

    y += DEBUG_LINE_HEIGHT;


    snprintf(
        text,
        sizeof(text),
        "  X: %.1f",
        state->enemy.x
    );

    debug_text(
        x,
        y,
        text
    );

    y += DEBUG_LINE_HEIGHT;


    snprintf(
        text,
        sizeof(text),
        "  Y: %.1f",
        state->enemy.y
    );

    debug_text(
        x,
        y,
        text
    );

    y += DEBUG_LINE_HEIGHT;


    snprintf(
        text,
        sizeof(text),
        "  Score: %d",
        state->enemy.score
    );

    debug_text(
        x,
        y,
        text
    );

    y += DEBUG_LINE_HEIGHT;


    /* --------------------------------------------------------
     * Entities
     * --------------------------------------------------------
     */

    debug_text(
        x,
        y,
        "ENTITIES"
    );

    y += DEBUG_LINE_HEIGHT;


    snprintf(
        text,
        sizeof(text),
        "  Balls: %d / %d",
        debug->ball_count,
        MAX_BALLS
    );

    debug_text(
        x,
        y,
        text
    );

    y += DEBUG_LINE_HEIGHT;


    snprintf(
        text,
        sizeof(text),
        "  Particles: %d / %d",
        debug->particle_count,
        MAX_PARTICLES
    );

    debug_text(
        x,
        y,
        text
    );

    y += DEBUG_LINE_HEIGHT;


    snprintf(
        text,
        sizeof(text),
        "  Powerups: %d / %d",
        debug->powerup_count,
        MAX_POWERUPS
    );

    debug_text(
        x,
        y,
        text
    );
}


/* ============================================================
 * COLLISION DEBUG DRAWING
 * ============================================================
 */

void
debug_draw_collisions(
    const SimulationState *state
)
{
    if (state == NULL)
        return;


    /*
     * Player.
     */

    DrawRectangleLines(
        (int)state->player.x,
        (int)state->player.y,
        (int)state->player.width,
        (int)state->player.height,
        RED
    );


    /*
     * Enemy.
     */

    DrawRectangleLines(
        (int)state->enemy.x,
        (int)state->enemy.y,
        (int)state->enemy.width,
        (int)state->enemy.height,
        RED
    );


    /*
     * Balls.
     */

    for (int i = 0;
         i < MAX_BALLS;
         ++i) {

        if (!state->balls.active[i])
            continue;


        DrawRectangleLines(
            (int)state->balls.x[i],
            (int)state->balls.y[i],
            BALL_SIZE,
            BALL_SIZE,
            GREEN
        );
    }
}


/* ============================================================
 * VELOCITY DEBUG DRAWING
 * ============================================================
 */

void
debug_draw_velocity(
    const SimulationState *state
)
{
    if (state == NULL)
        return;


    for (int i = 0;
         i < MAX_BALLS;
         ++i) {

        if (!state->balls.active[i])
            continue;


        float center_x =
            state->balls.x[i] +
            BALL_SIZE * 0.5f;

        float center_y =
            state->balls.y[i] +
            BALL_SIZE * 0.5f;


        float end_x =
            center_x +
            state->balls.vx[i] * 0.25f;

        float end_y =
            center_y +
            state->balls.vy[i] * 0.25f;


        DrawLine(
            (int)center_x,
            (int)center_y,
            (int)end_x,
            (int)end_y,
            YELLOW
        );
    }
}

