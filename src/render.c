#include "render.h"

#include "raylib.h"

#include "game.h"

static float
lerp_float(float a, float b, float t)
{
return a + (b - a) * t;
}

void
render_init(void)
{
/*
* Rendering currently uses raylib directly, so there is no
* renderer-specific initialization.
*/
}

void
render_shutdown(void)
{
/*
* No renderer-specific resources currently need to be released.
*/
}

void
render_frame(
    const RenderSnapshot *previous,
    const RenderSnapshot *current,
    float alpha
)
{
    BeginDrawing();

    ClearBackground(BLACK);

    /*
     * Court.
     */
    DrawRectangleLines(
        (int)COURT_LEFT,
        (int)COURT_TOP,
        (int)(COURT_RIGHT - COURT_LEFT),
        (int)(COURT_BOTTOM - COURT_TOP),
        WHITE
    );

    /*
     * Center line.
     */
    DrawLine(
        SCREEN_WIDTH / 2,
        (int)COURT_TOP,
        SCREEN_WIDTH / 2,
        (int)COURT_BOTTOM,
        WHITE
    );

    /*
     * Player paddle.
     */
    float player_x = lerp_float(
        previous->player_x,
        current->player_x,
        alpha
    );

    float player_y = lerp_float(
        previous->player_y,
        current->player_y,
        alpha
    );

    DrawRectangle(
        (int)player_x,
        (int)player_y,
        (int)PADDLE_WIDTH,
        (int)PADDLE_HEIGHT,
        WHITE
    );

    /*
     * Enemy paddle.
     */
    float enemy_x = lerp_float(
        previous->enemy_x,
        current->enemy_x,
        alpha
    );

    float enemy_y = lerp_float(
        previous->enemy_y,
        current->enemy_y,
        alpha
    );

    DrawRectangle(
        (int)enemy_x,
        (int)enemy_y,
        (int)PADDLE_WIDTH,
        (int)PADDLE_HEIGHT,
        WHITE
    );

    /*
     * Balls.
     *
     * Ball slots are stable. Iterate over the entire pool
     * and use the active flag to determine which slots exist.
     */
    for (int i = 0; i < MAX_BALLS; ++i) {

        if (!current->balls[i].active)
            continue;

        float x = current->balls[i].x;
        float y = current->balls[i].y;

        /*
         * Interpolate only when this same slot was active
         * in the previous snapshot.
         */
        if (previous->balls[i].active) {

            x = lerp_float(
                previous->balls[i].x,
                current->balls[i].x,
                alpha
            );

            y = lerp_float(
                previous->balls[i].y,
                current->balls[i].y,
                alpha
            );
        }

        DrawRectangle(
            (int)x,
            (int)y,
            (int)BALL_SIZE,
            (int)BALL_SIZE,
            WHITE
        );
    }

    EndDrawing();
}



