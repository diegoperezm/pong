#include "debug.h"
#include "raylib.h"

void debug_init(DebugState *debug)
{
    if (!debug) return;
    debug->enabled = false;
    debug->fps = 0.0f;
    debug->frame_time = 0.0f;
}

void debug_update(DebugState *debug, float raw_frame_time)
{
    if (!debug || !debug->enabled) return;
    debug->frame_time = raw_frame_time;
    if (raw_frame_time > 0.0f) debug->fps = 1.0f / raw_frame_time;
}

void debug_draw(const DebugState *debug, const AppView *view, const EventQueue *events)
{
    SimEvent latest;
    if (!debug || !debug->enabled) return;

    DrawRectangle(10, 10, 270, 124, Fade(BLACK, 0.86f));
    DrawRectangleLines(10, 10, 270, 124, GRAY);
    DrawText(TextFormat("FPS: %.1f", debug->fps), 20, 18, 17, GREEN);
    DrawText(TextFormat("Frame: %.2f ms", debug->frame_time * 1000.0f), 20, 39, 15, RAYWHITE);

    if (view && view->sim) {
        DrawText(TextFormat("Moves: %u", view->sim->move_count), 20, 59, 15, YELLOW);
        DrawText(TextFormat("Seed: %016llx", (unsigned long long)view->sim->seed), 20, 79, 12, LIGHTGRAY);
    }
    if (event_queue_recent(events, 0, &latest))
        DrawText(TextFormat("Event: %s", event_type_name(latest.type)), 20, 101, 13, SKYBLUE);
}
