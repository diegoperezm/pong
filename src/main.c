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
    /*
     * Input is sampled once per rendered frame.
     */
    input_sample(&input);

    /*
     * Accumulate real elapsed time.
     */
    double frame_time = GetFrameTime();

    if (frame_time > MAX_FRAME_TIME)
        frame_time = MAX_FRAME_TIME;

    accumulator += frame_time;

    /*
     * Fixed-timestep simulation.
     */
    while (accumulator >= SIM_DT) {
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

    /*
     * Interpolation factor between the previous and current
     * simulation states.
     */
    float alpha = (float)(accumulator / SIM_DT);

    render_frame(
        &previous,
        &current,
        alpha
    );
}

  simulation_shutdown();
  CloseWindow();

  return 0;
}

