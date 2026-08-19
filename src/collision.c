#include "collision.h"
#include "entities.h"
#include <math.h>
#include "log.h"

int
rectangles_overlap(
    float ax,
    float ay,
    float aw,
    float ah,

    float bx,
    float by,
    float bw,
    float bh
)
{
    return
        ax < bx + bw &&
        ax + aw > bx &&
        ay < by + bh &&
        ay + ah > by;
}


/* ------------------------------------------------------------
 * BALL / WALL
 * ------------------------------------------------------------
 */

static void
ball_wall_collision(
    SimulationState *state
)
{
    BallPool *balls = &state->balls;

    for (int i = 0; i < MAX_BALLS; ++i) {
      EntityHandle handle = {
         .index      = (uint32_t)i,
	 .generation = balls->slots[i].generation
      };

      if(!ball_is_valid(balls,handle))
         continue;

      if (balls->y[i] <= COURT_TOP) {
            balls->y[i] = COURT_TOP;
            balls->vy[i] = fabsf( balls->vy[i]);

          particles_spawn(
            &state->particles,
            balls->x[i],
            balls->y[i],
             4
            );
        }


        if (balls->y[i] + BALL_SIZE >= COURT_BOTTOM) {
            balls->y[i] = COURT_BOTTOM - BALL_SIZE;
            balls->vy[i] = -fabsf( balls->vy[i]);

            particles_spawn(
                &state->particles,
                balls->x[i],
                balls->y[i],
                4
            );
        }
    }
}


/* ------------------------------------------------------------
 * BALL / PADDLE
 * ------------------------------------------------------------
 */

static void
bounce_ball(
    SimulationState *state,
    EntityHandle handle,
    const Paddle *paddle
)
{
    BallPool *balls = &state->balls;

    if (!ball_is_valid(balls, handle))
        return;

    uint32_t ball = handle.index;

    float paddle_center = paddle->y + paddle->height / 2.0f;
    float ball_center   = balls->y[ball] + BALL_SIZE / 2.0f;
    float offset        = ball_center - paddle_center;

    // Normalize hit offset between -1.0 and 1.0 based on half paddle height
    float normalized_offset = offset / (paddle->height / 2.0f);
    if (normalized_offset < -1.0f) normalized_offset = -1.0f;
    if (normalized_offset >  1.0f) normalized_offset =  1.0f;

    /*
     * Increase speed on paddle bounce.
     */
    balls->speed[ball] += 20.0f;
    if (balls->speed[ball] > MAX_BALL_SPEED) {
        balls->speed[ball] = MAX_BALL_SPEED;
    }

    /*
     * Reverse horizontal direction.
     */
    float horizontal = (balls->vx[ball] < 0.0f) ? 1.0f : -1.0f;

    /*
     * Limit maximum deflection angle (e.g. max 60 degrees vertical influence).
     */
    float vertical = normalized_offset * 1.5f;

    /*
     * Normalize direction vector properly.
     */
    float length = sqrtf(horizontal * horizontal + vertical * vertical);
    if (length > 0.0f) {
        horizontal /= length;
        vertical   /= length;
    }

    /*
     * Apply new velocities.
     */
    balls->vx[ball] = horizontal * balls->speed[ball];
    balls->vy[ball] = vertical   * balls->speed[ball];

    LOG_COLLISION(
        "ball=%u bounce: vx=%.2f vy=%.2f speed=%.2f",
        ball,
        balls->vx[ball],
        balls->vy[ball],
        balls->speed[ball]
    );

    particles_spawn(
        &state->particles,
        balls->x[ball],
        balls->y[ball],
        8
    );
}

static void
ball_paddle_collision(SimulationState *state)
{
    BallPool *balls = &state->balls;

    for (int i = 0; i < MAX_BALLS; ++i) {

        EntityHandle handle = {
            .index = (uint32_t)i,
            .generation = balls->slots[i].generation
        };

        if (!ball_is_valid(balls, handle))
            continue;

        /*
         * Player paddle.
         */
        if (balls->vx[i] < 0.0f) {

            if (rectangles_overlap(
                    balls->x[i],
                    balls->y[i],
                    BALL_SIZE,
                    BALL_SIZE,

                    state->player.x,
                    state->player.y,
                    state->player.width,
                    state->player.height)) {

                balls->x[i] =
                    state->player.x +
                    state->player.width;

                LOG_COLLISION(
                 "ball=%d hit player paddle",
                 i);

                bounce_ball(
                    state,
                    handle,
                    &state->player
                );
            }
        }


        /*
         * Enemy paddle.
         */
        if (balls->vx[i] > 0.0f) {

            if (rectangles_overlap(
                    balls->x[i],
                    balls->y[i],
                    BALL_SIZE,
                    BALL_SIZE,

                    state->enemy.x,
                    state->enemy.y,
                    state->enemy.width,
                    state->enemy.height)) {

                balls->x[i] = state->enemy.x - BALL_SIZE;

                bounce_ball(
                    state,
                    handle,
                    &state->enemy
                );
            }
        }
    }
}




/* ------------------------------------------------------------
 * BALL / POWERUP
 * ------------------------------------------------------------
 */

static void
powerup_apply(
    SimulationState *state,
    int powerup,
    int ball
)
{
    BallPool *balls =
        &state->balls;

    PowerupPool *powerups =
        &state->powerups;


    switch (powerups->type[powerup]) {

        case POWERUP_SPEED:
        {
            balls->speed[ball] +=
                100.0f;


            if (balls->speed[ball] >
                MAX_BALL_SPEED) {

                balls->speed[ball] =
                    MAX_BALL_SPEED;
            }


            float direction =
                balls->vx[ball] < 0.0f
                    ? -1.0f
                    : 1.0f;


            balls->vx[ball] =
                direction *
                balls->speed[ball];

            break;
        }


        case POWERUP_MULTI_BALL:
        {
            /*
             * Create another ball.
             */
            EntityHandle new_ball =
                ball_create(
                    balls,

                    balls->x[ball],
                    balls->y[ball],

                    -balls->vx[ball],
                    balls->vy[ball] * 0.8f
                );


            (void)new_ball;

            break;
        }
    }


    particles_spawn(
        &state->particles,

        powerups->x[powerup],
        powerups->y[powerup],

        15
    );


    powerups->active[powerup] = 0;
}

/*
static void
powerup_collision(
    SimulationState *state
)
{
    BallPool *balls =
        &state->balls;

    PowerupPool *powerups =
        &state->powerups;


    for (int p = 0;
         p < MAX_POWERUPS;
         ++p) {

        if (!powerups->active[p])
            continue;


        for (int b = 0;
             b < MAX_BALLS;
             ++b) {

            if (!balls->active[b])
                continue;


            if (!rectangles_overlap(
                    balls->x[b],
                    balls->y[b],
                    BALL_SIZE,
                    BALL_SIZE,

                    powerups->x[p],
                    powerups->y[p],
                    POWERUP_SIZE,
                    POWERUP_SIZE))
                continue;


            powerup_apply(
                state,
                p,
                b
            );


            
           //   One ball can consume
           //   this power-up.
            
            break;
        }
    }
}
*/


void
collision_update(
    SimulationState *state
)
{
    ball_wall_collision(state);
    ball_paddle_collision(state);
//    powerup_collision(state);
}
