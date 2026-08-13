#include "game.h"

#include "entities.h"
#include "simulation.h"


static void
reset_paddles( SimulationState *state)
{
    state->player.x      = COURT_LEFT + 20.0f;
    state->player.y      = SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f;
    state->player.width  = PADDLE_WIDTH;
    state->player.height = PADDLE_HEIGHT;
    state->player.speed  = PLAYER_SPEED;

    state->enemy.x      = COURT_RIGHT - PADDLE_WIDTH - 20.0f;
    state->enemy.y      =  300;//SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f;
    state->enemy.width  = PADDLE_WIDTH;
    state->enemy.height = PADDLE_HEIGHT;
    state->enemy.speed  = AI_SPEED;
}


void
game_init(SimulationState *state)
{
    /*
     * Zero entire state.
     */
    *state = (SimulationState){0};

    state->mode = GAME_TITLE;

    reset_paddles(state);
    balls_clear(&state->balls);
    particles_clear(&state->particles);
    powerups_clear(&state->powerups);
}


void
game_start(SimulationState *state)
{
    state->mode         = GAME_PLAYING;
    state->game_time    = 0.0f;
    state->winner       = 0;
    state->player.score = 0;
    state->enemy.score  = 0;

    reset_paddles(state);
    balls_clear(&state->balls);
//    particles_clear(&state->particles);
//    powerups_clear(&state->powerups);

    state->powerup_timer = POWERUP_INTERVAL;

    ball_create(
        &state->balls,
        SCREEN_WIDTH / 2.0f - BALL_SIZE / 2.0f,
        SCREEN_HEIGHT / 2.0f - BALL_SIZE / 2.0f,
        INITIAL_BALL_SPEED, 
	100.0f
    );
}


void
game_update(
    SimulationState *state,
    const GameInput *input,
    float dt
)
{
    state->game_time += dt;

    switch (state->mode) {
        case GAME_TITLE:
            if (input->start)
                game_start(state);

            break;

        case GAME_PLAYING:
            if (input->pause) {
                state->mode = GAME_PAUSED;
                break;
            }

            simulation_update(
                state,
                input,
                dt
            );

            break;

        case GAME_PAUSED:
            if (input->pause)
                state->mode = GAME_PLAYING;
            break;


        case GAME_OVER:
            if (input->start)
                game_start(state);
            break;
    }
}


void
game_make_render_snapshot(
    const SimulationState *state,
    RenderSnapshot *snapshot
)
{
    /*
     * We explicitly copy only things
     * the renderer needs.
     */

    snapshot->player_x = state->player.x;
    snapshot->player_y = state->player.y;

    snapshot->enemy_x = state->enemy.x;
    snapshot->enemy_y = state->enemy.y;


    for (int i = 0; i < MAX_BALLS; ++i) {
        snapshot->ball_active[i] = state->balls.active[i];
        snapshot->ball_x[i]      = state->balls.x[i];

        snapshot->ball_y[i] = state->balls.y[i];
    }
}
