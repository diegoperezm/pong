#include "debug.h"
#include "config.h"
#include "render.h"
#include "raylib.h"

#include <stdarg.h>
#include <stdio.h>

#define DEBUG_FPS_SMOOTHING   0.10f  /* EMA weight of the newest sample   */
#define DEBUG_VELOCITY_SCALE  0.5f   /* seconds of travel the line shows  */

/* ============================================================
 * INIT / UPDATE
 * ============================================================
 */

void debug_init(DebugState *debug) {
    debug->enabled         = 0;
    debug->show_collisions = 0;
    debug->show_velocity   = 0;
    debug->frame_time      = 0.0f;
    debug->fps             = 0.0f;
    debug->ball_count      = 0;
    debug->particle_count  = 0;
    debug->powerup_count   = 0;
}

static int debug_count_balls(const RenderSnapshot *snapshot) {
    return snapshot->ball_count;
}

static int debug_count_particles(const RenderSnapshot *snapshot) {
    int count = 0;
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (snapshot->particles[i].active)
            ++count;
    }
    return count;
}

static int debug_count_powerups(const RenderSnapshot *snapshot) {
    int count = 0;
    for (int i = 0; i < MAX_POWERUPS; ++i) {
        if (snapshot->powerups[i].active)
            ++count;
    }
    return count;
}

void debug_update(DebugState *debug, const RenderSnapshot *current, float raw_frame_time) {
    if (debug == NULL || current == NULL || !debug->enabled) return;

    /* Instantaneous, uncapped: spikes stay visible. */
    debug->frame_time = raw_frame_time;

    /* FPS is smoothed so the number is readable. */
    if (raw_frame_time > 0.0f) {
        float instant_fps = 1.0f / raw_frame_time;
        debug->fps = (debug->fps <= 0.0f)
            ? instant_fps
            : debug->fps + (instant_fps - debug->fps) * DEBUG_FPS_SMOOTHING;
    }

    debug->ball_count     = debug_count_balls(current);
    debug->particle_count = debug_count_particles(current);
    debug->powerup_count  = debug_count_powerups(current);
}

/* ============================================================
 * TEXT PANEL
 * ============================================================
 */

typedef struct {
    int x;
    int y;
} DebugCursor;

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 2, 3)))
#endif
static void debug_line(DebugCursor *cursor, const char *fmt, ...) {
    char text[128];

    va_list args;
    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);

    DrawText(text, cursor->x, cursor->y, DEBUG_FONT_SIZE, RAYWHITE);
    cursor->y += DEBUG_LINE_HEIGHT;
}

static const char *debug_game_mode_name(GameMode mode) {
    switch (mode) {
        case GAME_TITLE:   return "TITLE";
        case GAME_PLAYING: return "PLAYING";
        case GAME_PAUSED:  return "PAUSED";
        case GAME_OVER:    return "GAME OVER";
        default:           return "UNKNOWN";
    }
}

static void debug_draw_panel(const DebugState *debug, const RenderSnapshot *current) {
    DrawRectangle(DEBUG_PANEL_X, DEBUG_PANEL_Y, DEBUG_PANEL_WIDTH, DEBUG_PANEL_HEIGHT, Fade(BLACK, 0.80f));
    DrawRectangleLines(DEBUG_PANEL_X, DEBUG_PANEL_Y, DEBUG_PANEL_WIDTH, DEBUG_PANEL_HEIGHT, RAYWHITE);

    DebugCursor cursor = {
        DEBUG_PANEL_X + DEBUG_PADDING,
        DEBUG_PANEL_Y + DEBUG_PADDING
    };

    debug_line(&cursor, "DEBUG");
    debug_line(&cursor, "FPS: %.1f", debug->fps);
    debug_line(&cursor, "Frame: %.3f ms", debug->frame_time * 1000.0f);
    debug_line(&cursor, "Mode: %s", debug_game_mode_name(current->mode));
    debug_line(&cursor, "Time: %.2f", current->game_time);
    debug_line(&cursor, "Player: (%.1f, %.1f)  score %d",
               current->player_x, current->player_y, current->player_score);
    debug_line(&cursor, "Enemy:  (%.1f, %.1f)  score %d",
               current->enemy_x, current->enemy_y, current->enemy_score);
    debug_line(&cursor, "Balls: %d / %d", debug->ball_count, MAX_BALLS);
    debug_line(&cursor, "Particles: %d / %d", debug->particle_count, MAX_PARTICLES);
    debug_line(&cursor, "Powerups: %d / %d", debug->powerup_count, MAX_POWERUPS);
}

/* ============================================================
 * WORLD OVERLAYS (snapshot-based, interpolated like the sprites)
 * ============================================================
 */

static bool debug_mode_has_world(GameMode mode) {
    return mode == GAME_PLAYING || mode == GAME_PAUSED;
}

/* Same (int) truncation render.c applies to the sprite, so outlines
 * land on the exact pixels of what is drawn. */
static void debug_ball_center(RenderVec2 pos, int *cx, int *cy) {
    *cx = (int)pos.x + (int)(BALL_SIZE * 0.5f);
    *cy = (int)pos.y + (int)(BALL_SIZE * 0.5f);
}

static void debug_draw_collisions(
    const RenderSnapshot *previous,
    const RenderSnapshot *current,
    float alpha)
{
    RenderVec2 player = render_player_position(previous, current, alpha);
    RenderVec2 enemy  = render_enemy_position(previous, current, alpha);

    DrawRectangleLines((int)player.x, (int)player.y,
                       (int)PADDLE_WIDTH, (int)PADDLE_HEIGHT, RED);
    DrawRectangleLines((int)enemy.x, (int)enemy.y,
                       (int)PADDLE_WIDTH, (int)PADDLE_HEIGHT, RED);

    /* The physics shape is a circle, so draw a circle. */
    for (int i = 0; i < MAX_BALLS; ++i) {
        RenderVec2 pos;
        if (!render_ball_position(previous, current, i, alpha, &pos))
            continue;

        int cx, cy;
        debug_ball_center(pos, &cx, &cy);
        DrawCircleLines(cx, cy, BALL_SIZE * 0.5f, GREEN);
    }
}

static void debug_draw_velocity(
    const RenderSnapshot *previous,
    const RenderSnapshot *current,
    float alpha)
{
    for (int i = 0; i < MAX_BALLS; ++i) {
        RenderVec2 pos;
        if (!render_ball_position(previous, current, i, alpha, &pos))
            continue;

        int cx, cy;
        debug_ball_center(pos, &cx, &cy);

        /* Velocity is piecewise-constant between steps: use the latest. */
        const RenderBall *ball = &current->balls[i];
        int ex = cx + (int)(ball->vx * DEBUG_VELOCITY_SCALE);
        int ey = cy + (int)(ball->vy * DEBUG_VELOCITY_SCALE);

        DrawLine(cx, cy, ex, ey, YELLOW);
    }
}

/* ============================================================
 * ENTRY POINT
 * ============================================================
 */

void debug_draw(
    const DebugState *debug,
    const RenderSnapshot *previous,
    const RenderSnapshot *current,
    float alpha)
{
    if (debug == NULL || previous == NULL || current == NULL)
        return;
    if (!debug->enabled)
        return;

    /* Overlays only where render_frame() actually draws entities. */
    if (debug_mode_has_world(current->mode)) {
        if (debug->show_collisions)
            debug_draw_collisions(previous, current, alpha);
        if (debug->show_velocity)
            debug_draw_velocity(previous, current, alpha);
    }

    debug_draw_panel(debug, current);
}
