#include "simulation.h"
#include "collision.h"
#include "entities.h"
//#include "raylib.h"
//#include <stdio.h> 
//#include "log.h"

static void
paddle_clamp(Paddle* paddle)
{
    if (paddle->y < COURT_TOP)
        paddle->y = COURT_TOP;

    float max_y = COURT_BOTTOM - paddle->height;

    if (paddle->y > max_y)
        paddle->y = max_y;
}


/* ------------------------------------------------------------
 * PLAYER
 * ------------------------------------------------------------
 */

static void
player_system(
    SimulationState *state,
    const GameInput *input,
    float dt
)
{
    float direction = 0.0f;

    if (input->up)   direction -= 1.0f;
    if (input->down) direction += 1.0f;

    state->player.y += direction * state->player.speed * dt;

    paddle_clamp(&state->player);
}


/* ------------------------------------------------------------
 * AI
 * ------------------------------------------------------------
 */

static void
enemy_system(
    SimulationState *state,
    float dt
)
{

  BallPool* balls = &state->balls;

  // check if existing target handle is still valid across frames
  if(!ball_is_valid(balls, state->ai_target)) {
     state->ai_target = INVALID_HANDLE;
     float best_x = -100000.0f;
     // find new ball
     for(uint32_t i = 0; i < MAX_BALLS; ++i) {
       if(!balls->slots[i].active) continue;

       if(balls->x[i] > best_x) {
         best_x = balls->x[i];
         state->ai_target = (EntityHandle){
		 .index = i,
                 .generation = balls->slots[i].generation
	 };
       } 
     }

  }

// perform ai movements using safe handle lookup
  if(!ball_is_valid(balls,state->ai_target)) return;

   uint32_t idx      = state->ai_target.index;
   float    ball_y   = balls->y[idx] + BALL_SIZE / 2.0f; 
   float    paddle_y = state->enemy.y + state->enemy.height / 2.0f;  

   if(ball_y < paddle_y) {
     state->enemy.y -= AI_SPEED * dt;
   } else if (ball_y > paddle_y) {
     state->enemy.y += AI_SPEED * dt;
   }

   /*
   LOG_AI(
      "target ball=%d x=%.1f y=%.1f",
      target,
      state->balls.x[target],
      state->balls.y[target]
    );

    LOG_AI(
      "paddle_y=%.1f ball_y=%.1f",
      paddle_y,
      ball_y
    );
    */
    paddle_clamp(&state->enemy);
}


/* ------------------------------------------------------------
 * BALL MOVEMENT
 * ------------------------------------------------------------
 */

static void
ball_movement_system( SimulationState *state, float dt)
{
    BallPool *balls = &state->balls;

    for (int i = 0; i < MAX_BALLS; ++i) {
        EntityHandle handle = {
            .index = (uint32_t)i,
            .generation = balls->slots[i].generation
        };

        if (!ball_is_valid(balls, handle))
            continue;

        balls->x[i] += balls->vx[i] * dt;
        balls->y[i] += balls->vy[i] * dt;
    }
}


/* ------------------------------------------------------------
 * SCORING
 * ------------------------------------------------------------
 */

static void 
scoring_system(SimulationState *state) {
    BallPool *balls = &state->balls;
    int active_balls = 0;

    for (uint32_t i = 0; i < MAX_BALLS; ++i) {
        if (!balls->slots[i].active) continue;

        EntityHandle handle = {
            .index = i,
            .generation = balls->slots[i].generation
        };

        if (balls->x[i] < COURT_LEFT - BALL_SIZE) {
            state->enemy.score++;
            particles_spawn(&state->particles, balls->x[i], balls->y[i], 20);
            ball_destroy(balls, handle); // Safely invalidates AI/Render handles
        } else if (balls->x[i] > COURT_RIGHT) {
            state->player.score++;
            particles_spawn(&state->particles, balls->x[i], balls->y[i], 20);
            ball_destroy(balls, handle);
        } else {
	    active_balls++;
	}
    }

    /*
     * If all balls are destroyed, respawn a new ball at center.
     */
    if (active_balls == 0) {
        ball_create(
          balls,
          SCREEN_WIDTH / 2.0f - BALL_SIZE / 2.0f,
          SCREEN_HEIGHT / 2.0f - BALL_SIZE / 2.0f,
          (state->player.score > state->enemy.score) ? -INITIAL_BALL_SPEED : INITIAL_BALL_SPEED,
            100.0f
        );
    }
}


/* ------------------------------------------------------------
 * POWERUP SPAWNING
 * ------------------------------------------------------------
 */

static void
powerup_system(
    SimulationState *state,
    float dt
)
{
    state->powerup_timer -= dt;

    if (state->powerup_timer <= 0.0f) {
        powerup_create( &state->powerups);
        state->powerup_timer = POWERUP_INTERVAL;
    }
}


/* ------------------------------------------------------------
 * MAIN SIMULATION PIPELINE
 * ------------------------------------------------------------
 */

void
simulation_update(
    SimulationState *state,
    const GameInput *input,
    float dt
)
{
    player_system(state, input, dt);
    enemy_system(state, dt);
    ball_movement_system(state, dt);
    collision_update(state);
    scoring_system(state);
    particles_update( &state->particles, dt);
 //   powerups_update( &state->powerups, dt);
 //   powerup_system( state, dt);
}
