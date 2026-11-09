#include "app.h"
#include "config.h"
#include "debug.h"
#include "input.h"
#include "log.h"
#include "raylib.h"
#include "render.h"

#include <time.h>

int main(void)
{
    App app;
    DebugState debug;

    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "15-Puzzle - Discrete Logic");
    SetExitKey(KEY_NULL);
    SetTargetFPS(TARGET_FPS);

    log_init();
    log_enable(PUZZLE_LOG_GAME);
    log_enable(PUZZLE_LOG_SIMULATION);
    debug_init(&debug);
    app_init(&app, (uint64_t)time(NULL));

    while (!WindowShouldClose()) {
        GameInput input = {0};
        float frame_time = GetFrameTime();
        AppView view;

        input_sample(&input);

        if (input.debug_toggle) debug.enabled = !debug.enabled;

        app_frame(&app, &input, frame_time);
        view = app_view(&app);

        debug_update(&debug, frame_time);

        BeginDrawing();
            render_frame(&view);
            debug_draw(&debug, &view, &app.events);
        EndDrawing();
    }

    app_shutdown(&app);
    CloseWindow();
    return 0;
}
