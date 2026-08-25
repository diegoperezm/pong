#include "raylib.h"

#include "config.h"
#include "types.h"

#include "input.h"
#include "game.h"
#include "render.h"
#include "debug.h"
#include "log.h"

int
main(void)
{
    InitWindow( SCREEN_WIDTH, SCREEN_HEIGHT, "Pong");
    SetTargetFPS(60);

    DebugState debug;
    SimulationState state;
    game_init(&state);
    log_init();
    debug_init(&debug);

    log_enable(PONG_LOG_GAME);
    log_enable(PONG_LOG_COLLISION);
    log_enable(PONG_LOG_ENTITY);
    
    log_enable(PONG_LOG_AI);
    log_enable(PONG_LOG_INPUT);
    log_enable(PONG_LOG_SIMULATION);
    log_enable(PONG_LOG_RENDER);

    /*
     * Two small render snapshots.
     *
     * These are NOT copies of the whole
     * simulation state.
     */
    RenderSnapshot previous;
    RenderSnapshot current;


    game_make_render_snapshot(
        &state,
        &previous
    );


    current = previous;
    GameInput input = {0};
    double accumulator = 0.0;

    while (!WindowShouldClose()) {

        /*
         * ----------------------------------------------------
         * 1. Measure real time.
         * ----------------------------------------------------
         */

        double frame_time = GetFrameTime();


        /*
         * Prevent giant simulation
         * steps after pauses/debugging.
         */
        if (frame_time > MAX_FRAME_TIME) {
            frame_time = MAX_FRAME_TIME;
        }

        /*
         * ----------------------------------------------------
         * 2. Sample platform input.
         * ----------------------------------------------------
         */

        input_sample(&input);
        accumulator += frame_time;


        if (IsKeyPressed(KEY_F3)) {
          debug.enabled = !debug.enabled;
        }


        /*
         * ----------------------------------------------------
         * 3. Fixed timestep simulation.
         * ----------------------------------------------------
         */

        while (accumulator >= SIM_DT) {

            /*
             * Current render state becomes
             * previous render state.
             */
            previous = current;

            /*
             * Advance authoritative state.
             */
            game_update(
                &state,
                &input,
                SIM_DT
            );


            /*
             * Extract only render data.
             */
            game_make_render_snapshot(
                &state,
                &current
            );


            accumulator -= SIM_DT;
        }


        /*
         * ----------------------------------------------------
         * 4. Interpolation.
         * ----------------------------------------------------
         */

        float alpha = (float)(accumulator/SIM_DT);


        /*
         * ----------------------------------------------------
         * 5. Render.
         * ----------------------------------------------------
         */

        render_game(
            &state,
            &previous,
            &current,
            alpha
        );

        debug_update(
          &debug,
          &state,
          GetFrameTime()
        );

        debug_draw(
          &debug,
          &state
        );
    }


    CloseWindow();
    return 0;
}
