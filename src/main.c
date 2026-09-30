#include "raylib.h"
#include "game.h"
#include "input.h"
#include "render.h"
#include "simulation.h"
#include "types.h"

int main(void) {
    InitWindow(
        SCREEN_WIDTH,
        SCREEN_HEIGHT,
        "Pong"
    );

    SetTargetFPS(60);

    SimulationState state;
    GameInput input;

    RenderSnapshot previous = {0};
    RenderSnapshot current  = {0};

    game_init(&state);

    game_make_render_snapshot(&state, &previous);
    game_make_render_snapshot(&state, &current);

    double accumulator = 0.0;

    while (!WindowShouldClose()) {
        input_sample(&input);

        double frame_time = GetFrameTime();
        if (frame_time > MAX_FRAME_TIME)
            frame_time = MAX_FRAME_TIME;

        GameMode prev_mode = state.mode;

        // Process state transitions directly through game_update
        if (state.mode != GAME_PLAYING || input.pause) {
            game_update(&state, &input, 0.0f);
        }

        // On any state transition, immediately synchronize render snapshots
        if (state.mode != prev_mode) {
            accumulator = 0.0;
            game_make_render_snapshot(&state, &current);
            previous = current;
        }

        // Fixed-timestep simulation loop
        if (state.mode == GAME_PLAYING) {
            accumulator += frame_time;

            while (state.mode == GAME_PLAYING && accumulator >= SIM_DT) {
                previous = current;

                game_update(
                    &state,
                    &input,
                    (float)SIM_DT
                );
                game_make_render_snapshot(
                    &state,
                    &current
                );

                accumulator -= SIM_DT;
            }

            // Capture state change triggered during simulation (e.g. GAME_OVER)
            if (state.mode != GAME_PLAYING) {
                accumulator = 0.0;
                game_make_render_snapshot(&state, &current);
                previous = current;
            }
        }

        float alpha = (state.mode == GAME_PLAYING) ? (float)(accumulator / SIM_DT) : 1.0f;

        render_frame(
            &previous,
            &current,
            alpha
        );
    }

    simulation_shutdown(&state);
    CloseWindow();

    return 0;
}
