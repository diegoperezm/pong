#include "render.h"

#include "raylib.h"


static float
lerp( float a, float b, float t) 
{
    return a + (b - a) * t;
}


/* ------------------------------------------------------------
 * COURT
 * ------------------------------------------------------------
 */

static void
draw_court(void)
{
    for ( int y = (int)COURT_TOP; y < (int)COURT_BOTTOM; y += 20) {

        DrawRectangle(
            SCREEN_WIDTH / 2 - 2,
            y,
            4,
            10,
            WHITE
        );
    }
}


/* ------------------------------------------------------------
 * PADDLES
 * ------------------------------------------------------------
 */

static void
draw_paddles(
    const SimulationState *state,
    const RenderSnapshot *previous,
    const RenderSnapshot *current,
    float alpha
)
{
    (void)state;
    float player_x = lerp(previous->player_x, current->player_x, alpha);


    float player_y = lerp( previous->player_y, current->player_y, alpha);

    float enemy_x  = lerp( previous->enemy_x, current->enemy_x, alpha);


    float enemy_y  = lerp( previous->enemy_y, current->enemy_y, alpha);


    DrawRectangle(
        (int)player_x,
        (int)player_y,

        (int)PADDLE_WIDTH,
        (int)PADDLE_HEIGHT,

        WHITE
    );


    DrawRectangle(
        (int)enemy_x,
        (int)enemy_y,

        (int)PADDLE_WIDTH,
        (int)PADDLE_HEIGHT,

        WHITE
    );
}


/* ------------------------------------------------------------
 * BALLS
 * ------------------------------------------------------------
 */

static void
draw_balls(
    const RenderSnapshot *previous,
    const RenderSnapshot *current,
    float alpha
)
{
    for (int i = 0; i < MAX_BALLS; ++i) {
        /*
         * Ball didn't exist in either
         * snapshot.
         */
        if (!current->ball_active[i])
            continue;


        float x;
        float y;


        /*
         * Newly spawned ball:
         *
         * We cannot interpolate from
         * a non-existing object.
         */
        if (!previous->ball_active[i]) {

            x = current->ball_x[i];
            y = current->ball_y[i];

        } else {
            x = lerp( previous->ball_x[i], current->ball_x[i], alpha);
            y = lerp( previous->ball_y[i], current->ball_y[i], alpha);
        }


        DrawRectangle(
            (int)x,
            (int)y,

            (int)BALL_SIZE,
            (int)BALL_SIZE,

            WHITE
        );
    }
}


/* ------------------------------------------------------------
 * PARTICLES
 * ------------------------------------------------------------
 */

static void
draw_particles(
    const SimulationState *state
)
{
    const ParticlePool *particles = &state->particles;


    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!particles->active[i])
            continue;


        float life = particles->lifetime[i] / particles->max_lifetime[i];

        int alpha = (int)(life * 255.0f);


        if (alpha < 0)
            alpha = 0;


        if (alpha > 255)
            alpha = 255;


        Color color = {
            255,
            255,
            255,
            (unsigned char)alpha
        };


        DrawRectangle(
            (int)particles->x[i],
            (int)particles->y[i],

            (int)particles->size[i],
            (int)particles->size[i],

            color
        );
    }
}


/* ------------------------------------------------------------
 * POWERUPS
 * ------------------------------------------------------------
 */

static void
draw_powerups(
    const SimulationState *state
)
{
    const PowerupPool *powerups =
        &state->powerups;


    for (int i = 0;
         i < MAX_POWERUPS;
         ++i) {

        if (!powerups->active[i])
            continue;


        Color color;


        if (powerups->type[i] ==
            POWERUP_SPEED) {

            color = RED;
        }

        else {

            color = BLUE;
        }


        DrawRectangle(
            (int)powerups->x[i],
            (int)powerups->y[i],

            (int)POWERUP_SIZE,
            (int)POWERUP_SIZE,

            color
        );
    }
}


/* ------------------------------------------------------------
 * UI
 * ------------------------------------------------------------
 */

static void
draw_score(
    const SimulationState *state
)
{
    DrawText(
        TextFormat(
            "%d",
            state->player.score
        ),

        300,
        30,

        40,

        WHITE
    );


    DrawText(
        TextFormat(
            "%d",
            state->enemy.score
        ),

        480,
        30,

        40,

        WHITE
    );
}


/* ------------------------------------------------------------
 * SCREENS
 * ------------------------------------------------------------
 */

static void
draw_title(void)
{
    DrawText(
        "PONG",
        350,
        120,
        40,
        WHITE
    );


    DrawText(
        "PRESS ENTER",
        315,
        180,
        20,
        WHITE
    );
}


static void
draw_pause(void)
{
    DrawText(
        "PAUSED",
        350,
        200,
        25,
        WHITE
    );
}


static void
draw_game_over(
    const SimulationState *state
)
{
    const char *text =
        state->winner == 1
            ? "YOU WIN"
            : "YOU LOSE";


    DrawText(
        text,
        335,
        120,
        30,
        WHITE
    );


    DrawText(
        TextFormat(
            "%d - %d",
            state->player.score,
            state->enemy.score
        ),

        365,
        170,
        20,
        WHITE
    );


    DrawText(
        "PRESS ENTER",
        325,
        210,
        20,
        WHITE
    );
}


/* ------------------------------------------------------------
 * MAIN RENDER
 * ------------------------------------------------------------
 */

void
render_game(
    const SimulationState *state,
    const RenderSnapshot *previous,
    const RenderSnapshot *current,
    float alpha
)
{
    BeginDrawing();

    ClearBackground(BLACK);


    switch (state->mode) {

        case GAME_TITLE:

            draw_title();

            break;


        case GAME_PLAYING:

            draw_court();

            draw_powerups(state);

            draw_paddles(
                state,
                previous,
                current,
                alpha
            );

            draw_balls(
                previous,
                current,
                alpha
            );

            draw_particles(state);

            draw_score(state);

            break;


        case GAME_PAUSED:

            draw_court();

            draw_powerups(state);

            draw_paddles(
                state,
                previous,
                current,
                alpha
            );

            draw_balls(
                previous,
                current,
                alpha
            );

            draw_particles(state);

            draw_score(state);

            draw_pause();

            break;


        case GAME_OVER:

            draw_game_over(state);

            break;
    }


    EndDrawing();
}
