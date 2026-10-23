#include "debug.h"
#include "config.h"
#include "render.h"
#include "raylib.h"

#include <stdarg.h>
#include <stdio.h>

#define DEBUG_FPS_SMOOTHING   0.10f  /* EMA weight of the newest sample */

void debug_init(DebugState *debug) {
    debug->enabled        = 0;
    debug->frame_time     = 0.0f;
    debug->fps            = 0.0f;
    debug->entity_count   = 0;
    debug->particle_count = 0;
}

void debug_update(DebugState *debug, const RenderSnapshot *current, float raw_frame_time) {
    if (debug == NULL || current == NULL || !debug->enabled) return;

    debug->frame_time = raw_frame_time;

    if (raw_frame_time > 0.0f) {
        float instant_fps = 1.0f / raw_frame_time;
        debug->fps = (debug->fps <= 0.0f)
            ? instant_fps
            : debug->fps + (instant_fps - debug->fps) * DEBUG_FPS_SMOOTHING;
    }
}

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 6, 7)))
#endif
static void text_line(int *x, int *y, int font, int line_height, Color color,
                      const char *fmt, ...) {
    char text[160];
    va_list args;
    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);

    DrawText(text, *x, *y, font, color);
    *y += line_height;
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

    int x = DEBUG_PANEL_X + DEBUG_PADDING;
    int y = DEBUG_PANEL_Y + DEBUG_PADDING;
    const int F = DEBUG_FONT_SIZE, L = DEBUG_LINE_HEIGHT;

    text_line(&x, &y, F, L, RAYWHITE, "DEBUG");
    text_line(&x, &y, F, L, RAYWHITE, "FPS: %.1f", debug->fps);
    text_line(&x, &y, F, L, RAYWHITE, "Frame: %.3f ms", debug->frame_time * 1000.0f);
    text_line(&x, &y, F, L, RAYWHITE, "Mode: %s", debug_game_mode_name(current->mode));
    text_line(&x, &y, F, L, RAYWHITE, "Time: %.2f", current->game_time);
}

void debug_draw(
    const DebugState *debug,
    const RenderSnapshot *previous,
    const RenderSnapshot *current,
    float alpha,
    const DebugHistory *history)
{
    (void)previous;
    (void)alpha;
    (void)history;

    if (debug == NULL || current == NULL)
        return;
    if (!debug->enabled)
        return;

    debug_draw_panel(debug, current);
}
