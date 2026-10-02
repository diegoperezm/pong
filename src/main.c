#include "raylib.h"
#include "game.h"
#include "input.h"
#include "log.h"
#include "debug.h"
#include "render.h"
#include "simulation.h"
#include "types.h"

// Make both snapshots reflect the current state, so the next frame
// renders it without interpolating from a stale previous state.
static void
snapshots_reset(
    const SimulationState *state,
    RenderSnapshot *previous,
    RenderSnapshot *current)
{
    game_make_render_snapshot(state, current);
    *previous = *current;
}

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

    log_init();
    log_enable(PONG_LOG_GAME);
    log_enable(PONG_LOG_SIMULATION);
    log_enable(PONG_LOG_ENTITY);
    
    DebugState debug;
    debug_init(&debug);
    debug.enabled = 1;
    debug.show_collisions = 1;
    debug.show_velocity = 1;

    snapshots_reset(&state, &previous, &current);

    double accumulator = 0.0;

    while (!WindowShouldClose()) {
        input_sample(&input);

        // Real duration of the last frame: debug readout only.
        double raw_frame_time = GetFrameTime();

        // Clamped copy: drives the simulation (spiral-of-death guard).
        double frame_time = raw_frame_time;
        if (frame_time > MAX_FRAME_TIME)
            frame_time = MAX_FRAME_TIME;

        // 1. Once per frame: player-driven mode transitions.
        GameMode mode_before = state.mode;
        game_handle_input(&state, &input);

        if (state.mode != mode_before) {
            accumulator = 0.0;
            snapshots_reset(&state, &previous, &current);
        }

        // 2. Fixed-timestep simulation: held input (up/down) only.
        if (state.mode == GAME_PLAYING) {
            accumulator += frame_time;

            while (state.mode == GAME_PLAYING && accumulator >= SIM_DT) {
                previous = current;

                game_step(&state, &input, (float)SIM_DT);
                game_make_render_snapshot(&state, &current);

                accumulator -= SIM_DT;
            }

            // The simulation itself ended the match (GAME_OVER).
            if (state.mode != GAME_PLAYING) {
                accumulator = 0.0;
                snapshots_reset(&state, &previous, &current);
            }
        }

        float alpha = (state.mode == GAME_PLAYING) ? (float)(accumulator / SIM_DT) : 1.0f;

        // After the simulation, so entity counts match what is drawn this frame.
        debug_update(&debug, &state, (float)raw_frame_time);

        BeginDrawing();
            ClearBackground(BLACK);

            // 1. Game world
            render_frame(&previous, &current, alpha);

            // 2. Debug overlay: same snapshots and alpha as the world above
            debug_draw(&debug, &state, &previous, &current, alpha);

        EndDrawing();
    }

    simulation_shutdown(&state);
    CloseWindow();

    return 0;
}


