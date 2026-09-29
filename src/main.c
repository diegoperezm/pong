#include "raylib.h"
#include "game.h"
#include "input.h"
#include "render.h"
#include "simulation.h"
#include "types.h"

int
main(void)
{
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

  game_make_render_snapshot(
    &state,
    &previous
  );

  game_make_render_snapshot(
    &state,
    &current
  );


  double accumulator = 0.0;

while (!WindowShouldClose()) {
    input_sample(&input);

    double frame_time = GetFrameTime();
    if (frame_time > MAX_FRAME_TIME)
        frame_time = MAX_FRAME_TIME;

    // Game module owns all state transitions and input logic
    if (game_handle_input(&state, &input)) {
        accumulator = 0.0; // Prevent catch-up jumps on state changes
    }

    accumulator += frame_time;

    /*
     * Fixed-timestep simulation loop
     */
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

    // Ensure snapshots are fresh if paused, on title, or game over
    if (state.mode != GAME_PLAYING) {
        game_make_render_snapshot(&state, &current);
        previous = current;
    }

    float alpha = (float)(accumulator / SIM_DT);

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

