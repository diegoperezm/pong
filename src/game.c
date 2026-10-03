#include "game.h"
#include <string.h>
#include "entities.h"
#include "simulation.h"

static
void reset_paddles(SimulationState *state) {
    state->player.x      = COURT_LEFT + 20.0f;
    state->player.y      = SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f;
    state->player.width  = PADDLE_WIDTH;
    state->player.height = PADDLE_HEIGHT;
    state->player.speed  = PLAYER_SPEED;
  
    state->enemy.x      = COURT_RIGHT - PADDLE_WIDTH - 20.0f;
    state->enemy.y      = SCREEN_HEIGHT / 2.0f - PADDLE_HEIGHT / 2.0f;
    state->enemy.width  = PADDLE_WIDTH;
    state->enemy.height = PADDLE_HEIGHT;
    state->enemy.speed  = PLAYER_SPEED;
}

void
game_init(SimulationState *state) {
    memset(state, 0, sizeof(*state));
    state->mode = GAME_TITLE;
    
    reset_paddles(state);

    simulation_init(state);
    ball_pool_init(&state->balls);
    particles_clear(&state->particles);
    powerups_clear(&state->powerups);
}

void
game_start(SimulationState *state) {
    state->mode      = GAME_PLAYING;
    state->game_time = 0.0f;
    state->winner    = 0;

    // Explicitly reset scores for a fresh match
    state->player.score = 0;
    state->enemy.score  = 0;

    reset_paddles(state);

    particles_clear(&state->particles);
    powerups_clear(&state->powerups);

    state->powerup_timer = 0.0f;

    // simulation_reset repositions paddle bodies and clears the balls
    simulation_reset(state);

    ball_create(
        &state->balls,
        state->world,
        SCREEN_WIDTH / 2.0f - BALL_SIZE / 2.0f,
        SCREEN_HEIGHT / 2.0f - BALL_SIZE / 2.0f,
        1.0f
    );
}

/* ============================================================
 * STATE MACHINE (once per frame)
 * ============================================================
 */

void
game_handle_input(SimulationState *state, const GameInput *input)
{
    switch (state->mode) {
        case GAME_TITLE:
        case GAME_OVER:
            if (input->start)
                game_start(state);
            break;

        case GAME_PLAYING:
            if (input->pause)
                state->mode = GAME_PAUSED;
            break;

        case GAME_PAUSED:
            if (input->pause)
                state->mode = GAME_PLAYING;
            break;
    }
}

/* ============================================================
 * FIXED STEP (zero or more times per frame)
 * ============================================================
 */

void
game_step(SimulationState *state, const GameInput *input, float dt)
{
    if (state->mode != GAME_PLAYING)
        return;

    state->game_time += dt;
    simulation_update(state, input, dt);
}

/* ============================================================
 * RENDER SNAPSHOT
 * ============================================================
 */

void
game_make_render_snapshot(const SimulationState *state, RenderSnapshot *snapshot) {
    snapshot->player_x     = state->player.x;
    snapshot->player_y     = state->player.y;
    snapshot->player_score = state->player.score;

    snapshot->enemy_x      = state->enemy.x;
    snapshot->enemy_y      = state->enemy.y;
    snapshot->enemy_score  = state->enemy.score;
  
    snapshot->mode         = state->mode;
    snapshot->winner       = state->winner;
    snapshot->game_time    = state->game_time;
    snapshot->ball_count   = (int)state->balls.count;
  
    // 1. Deactivate every ball slot in the snapshot
    for (int i = 0; i < MAX_BALLS; ++i) {
        snapshot->balls[i].active = false;
    }

    // 2. Map each live ball to its stable sparse slot (dense_to_sparse)
    for (uint32_t d = 0; d < state->balls.count; ++d) {
        uint32_t slot_idx = state->balls.dense_to_sparse[d];
        if (slot_idx < MAX_BALLS) {
            snapshot->balls[slot_idx].active     = true;
            snapshot->balls[slot_idx].x          = state->balls.x[d];
            snapshot->balls[slot_idx].y          = state->balls.y[d];
            snapshot->balls[slot_idx].vx         = state->balls.vx[d];
            snapshot->balls[slot_idx].vy         = state->balls.vy[d];
            snapshot->balls[slot_idx].generation = state->balls.slots[slot_idx].generation;
        }
    }

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        snapshot->particles[i].active       = state->particles.active[i];
        snapshot->particles[i].x            = state->particles.x[i];
        snapshot->particles[i].y            = state->particles.y[i];
        snapshot->particles[i].lifetime     = state->particles.lifetime[i];
        snapshot->particles[i].max_lifetime = state->particles.max_lifetime[i];
        snapshot->particles[i].size         = state->particles.size[i];
    }

    for (int i = 0; i < MAX_POWERUPS; ++i) {
        snapshot->powerups[i].active = state->powerups.active[i];
        snapshot->powerups[i].x      = state->powerups.x[i];
        snapshot->powerups[i].y      = state->powerups.y[i];
        snapshot->powerups[i].type   = state->powerups.type[i];
    }
}
