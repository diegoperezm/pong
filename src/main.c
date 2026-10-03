#include <time.h>

#include "raylib.h"
#include "app.h"
#include "input.h"
#include "log.h"
#include "debug.h"
#include "render.h"

/* main() is only the platform shell: window, input sampling, drawing.
 * Everything about the game itself lives in the app layer. */

int main(void) {
    InitWindow(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        "Pong"
    );

    /* ESC is the pause key; without this raylib would close the window. */
    SetExitKey(KEY_NULL);
    SetTargetFPS(60);

    /* Logging must be configured BEFORE anything that logs. */
    log_init();
    log_enable(PONG_LOG_GAME);
    log_enable(PONG_LOG_SIMULATION);
    log_enable(PONG_LOG_ENTITY);

    /* ~100 KB (two snapshots): keep it off the stack. Zero-initialised. */
    static App app;
    app_init(&app, (uint64_t)time(NULL));

    DebugState debug;
    debug_init(&debug);
    debug.enabled = 1;
    debug.show_collisions = 1;
    debug.show_velocity = 1;

    while (!WindowShouldClose()) {
        GameInput input;
        input_sample(&input);

        /* Real duration of the last frame: debug readout only. */
        double raw_frame_time = GetFrameTime();

        /* Clamped copy: drives the simulation (spiral-of-death guard). */
        double frame_time = raw_frame_time;
        if (frame_time > MAX_FRAME_TIME)
            frame_time = MAX_FRAME_TIME;

        if (input.debug_toggle)
            debug.enabled = !debug.enabled;

        app_frame(&app, &input, frame_time);

        AppView view = app_view(&app);
        DebugHistory history = app_debug_history(&app);

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
