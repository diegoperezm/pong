#include <time.h>
#include "raylib.h"
#include "config.h"
#include "app.h"
#include "input.h"
#include "log.h"
#include "debug.h"
#include "render.h"

int main(void) {
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "15-Puzzle Physics Simulation");
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    log_init();
    log_enable(PUZZLE_LOG_GAME);
    log_enable(PUZZLE_LOG_SIMULATION);

    static App app;
    app_init(&app, (uint64_t)time(NULL));

    DebugState debug;
    debug_init(&debug);

    while (!WindowShouldClose()) {
        GameInput input;
        input_sample(&input);

        double raw_frame_time = GetFrameTime();
        double frame_time = raw_frame_time > MAX_FRAME_TIME ? MAX_FRAME_TIME : raw_frame_time;

        if (input.debug_toggle)
            debug.enabled = !debug.enabled;

        app_frame(&app, &input, frame_time);

        AppView view = app_view(&app);
        DebugHistory history = { .events = &app.events, .tick = app.sim.tick, .seed = app.sim.seed, .status = app.status };

        debug_update(&debug, view.current, (float)raw_frame_time);

        BeginDrawing();
            ClearBackground(BLACK);
            render_frame(view.previous, view.current, view.alpha);
            debug_draw(&debug, view.previous, view.current, view.alpha, &history);
        EndDrawing();
    }

    app_shutdown(&app);
    CloseWindow();
    return 0;
}
