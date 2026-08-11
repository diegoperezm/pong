#include "simulation.h"

#include "collision.h"
#include "entities.h"

#include "raylib.h"


static void
paddle_clamp(
    Paddle *paddle
)
{
    if (paddle->y < COURT_TOP)
        paddle->y = COURT_TOP;


    float max_y =
        COURT_BOTTOM -
        paddle->height;


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


    if (input->up)
        direction -= 1.0f;


    if (input->down)
        direction += 1.0f;


    state->player.y +=
        direction *
        state->player.speed *
        dt;


    paddle_clamp(
        &state->player
    );
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
    int target = -1;

    float best_x =
        -100000.0f;


    for (int i = 0;
         i < MAX_BALLS;
         ++i) {

        if (!state->balls.active[i])
            continue;


        if (state->balls.x[i] >
            best_x) {

            best_x =
                state->balls.x[i];

            target = i;
        }
    }


    if (target < 0)
        return;


    float ball_y =
        state->balls.y[target] +
        BALL_SIZE / 2.0f;


    float paddle_y =
        state->enemy.y +
        state->enemy.height / 2.0f;


    if (ball_y < paddle_y)
        state->enemy.y -=
            AI_SPEED * dt;


    else if (ball_y > paddle_y)
        state->enemy.y +=
            AI_SPEED * dt;


    paddle_clamp(
        &state->enemy
    );
}


/* ------------------------------------------------------------
 * BALL MOVEMENT
 * ------------------------------------------------------------
 */

static void
ball_movement_system(
    SimulationState *state,
    float dt
)
{
    BallPool *balls =
        &state->balls;


    for (int i = 0;
         i < MAX_BALLS;
         ++i) {

        if (!balls->active[i])
            continue;


        balls->x[i] +=
            balls->vx[i] * dt;


        balls->y[i] +=
            balls->vy[i] * dt;
    }
}


/* ------------------------------------------------------------
 * SCORING
 * ------------------------------------------------------------
 */

static void
scoring_system(
    SimulationState *state
)
{
    BallPool *balls =
        &state->balls;


    int active_balls = 0;


    for (int i = 0;
         i < MAX_BALLS;
         ++i) {

        if (!balls->active[i])
            continue;


        ++active_balls;


        /*
         * Enemy scores.
         */
        if (balls->x[i] <
            COURT_LEFT - BALL_SIZE) {

            state->enemy.score++;


            particles_spawn(
                &state->particles,

                balls->x[i],
                balls->y[i],

                20
            );


            ball_destroy(
                balls,
                i
            );
        }


        /*
         * Player scores.
         */
        else if (
            balls->x[i] >
            COURT_RIGHT) {

            state->player.score++;


            particles_spawn(
                &state->particles,

                balls->x[i],
                balls->y[i],

                20
            );


            ball_destroy(
                balls,
                i
            );
        }
    }


    if (state->player.score >=
        WINNING_SCORE) {

        state->winner = 1;

        state->mode =
            GAME_OVER;

        return;
    }


    if (state->enemy.score >=
        WINNING_SCORE) {

        state->winner = 2;

        state->mode =
            GAME_OVER;

        return;
    }


    /*
     * Count again after removals.
     */
    active_balls = 0;


    for (int i = 0;
         i < MAX_BALLS;
         ++i) {

        if (balls->active[i])
            ++active_balls;
    }


    /*
     * Guarantee at least one ball.
     */
    if (active_balls == 0) {

        float direction =
            GetRandomValue(0, 1)
                ? 1.0f
                : -1.0f;


        ball_create(
            balls,

            SCREEN_WIDTH / 2.0f,
            SCREEN_HEIGHT / 2.0f,

            INITIAL_BALL_SPEED *
                direction,

            (float)GetRandomValue(
                -100,
                100
            )
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

        powerup_create(
            &state->powerups
        );


        state->powerup_timer =
            POWERUP_INTERVAL;
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
    player_system(
        state,
        input,
        dt
    );


    enemy_system(
        state,
        dt
    );


    ball_movement_system(
        state,
        dt
    );


    collision_update(
        state
    );


    scoring_system(
        state
    );


    particles_update( &state->particles, dt);


    powerups_update(
        &state->powerups,
        dt
    );


    powerup_system(
        state,
        dt
    );
}
