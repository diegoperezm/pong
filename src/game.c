#include "game.h"
#include <string.h>
#include "entities.h"
#include "simulation.h"

static void
reset_paddles(SimulationState *state)
{
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
game_init(SimulationState *state)
{
  memset( state, 0, sizeof(*state));
  state->mode = GAME_TITLE;
  reset_paddles(state);
  balls_clear(&state->balls);
  particles_clear( &state->particles);
  powerups_clear( &state->powerups);
  simulation_init(state); 
}

void
game_start(SimulationState *state)
{
  state->mode = GAME_PLAYING;
  state->game_time = 0.0f;
  state->winner = 0;

  reset_paddles(state);
  balls_clear(&state->balls);
  particles_clear(&state->particles);
  powerups_clear(&state->powerups);

  state->powerup_timer = 0.0f;
  simulation_reset(state);

  
  ball_create(
       &state->balls,
       SCREEN_WIDTH / 2.0f - BALL_SIZE / 2.0f,
       SCREEN_HEIGHT / 2.0f - BALL_SIZE / 2.0f,
       INITIAL_BALL_SPEED
   );

}

void
game_update(
  SimulationState *state,
  const GameInput *input,
  float dt
)
{
  if (state->mode == GAME_TITLE) {
    if (input->start)
      game_start(state);
  return;
  }
  
  if (state->mode != GAME_PLAYING)
    return;
  
  state->game_time += dt;
  simulation_update(
    state,
    input,
    dt
  );
}

void
game_make_render_snapshot(
  const SimulationState* state,
  RenderSnapshot* snapshot
)
{
  snapshot->player_x     = state->player.x;
  snapshot->player_y     = state->player.y;
  snapshot->player_score = state->player.score;

  snapshot->enemy_x      = state->enemy.x;
  snapshot->enemy_y      = state->enemy.y;
  snapshot->enemy_score  = state->enemy.score;
  
  snapshot->mode         = state->mode;
  snapshot->winner       = state->winner;
  snapshot->ball_count   = 0;
  
  for (int i = 0; i < MAX_BALLS; ++i) {
    snapshot->balls[i].active = state->balls.slots[i].active;
    snapshot->balls[i].x      = state->balls.x[i];
    snapshot->balls[i].y      = state->balls.y[i];
    
    if (snapshot->balls[i].active)
      snapshot->ball_count++;
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
     snapshot->powerups[i].x = state->powerups.x[i];
     snapshot->powerups[i].y = state->powerups.y[i];
     snapshot->powerups[i].type = state->powerups.type[i];
  }
}
