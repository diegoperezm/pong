#include "debug.h"
#include "raylib.h"
#include <stdio.h>

void debug_init(DebugState *debug) {
    debug->enabled = 0;
    debug->fps = 0.0f;
    debug->frame_time = 0.0f;
}

void debug_update(DebugState *debug, const RenderSnapshot *current, float raw_frame_time) {
    (void)current;
    if (!debug->enabled) return;
    debug->frame_time = raw_frame_time;
    if (raw_frame_time > 0.0f) {
        debug->fps = 1.0f / raw_frame_time;
    }
}

void debug_draw(const DebugState *debug, const RenderSnapshot *previous, const RenderSnapshot *current, float alpha, const DebugHistory *history) {
    (void)previous;
    (void)current;
    (void)alpha;
    if (!debug->enabled) return;

    DrawRectangle(10, 10, 240, 105, Fade(BLACK, 0.85f));
    DrawRectangleLines(10, 10, 240, 105, RAYWHITE);
    DrawText(TextFormat("FPS: %.1f", debug->fps), 20, 18, 18, GREEN);
    DrawText(TextFormat("Frame: %.2f ms", debug->frame_time * 1000.0f), 20, 38, 16, RAYWHITE);
    if (history) {
        DrawText(TextFormat("Tick: %u", history->tick), 20, 58, 15, YELLOW);
        DrawText(TextFormat("Seed: %016llx", (unsigned long long)history->seed), 20, 78, 12, LIGHTGRAY);
    }
}
