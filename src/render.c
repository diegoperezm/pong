#include "render.h"
#include "raylib.h"

static float lerp_float(float a, float b, float t) {
    return a + (b - a) * t;
}

// SCREENS
static void draw_title(void) {
    const char *title = "GAME TITLE";
    const char *prompt = "PRESS ENTER";

    DrawText(title, (SCREEN_WIDTH / 2) - (MeasureText(title, 40) / 2), 120, 40, WHITE);
    DrawText(prompt, (SCREEN_WIDTH / 2) - (MeasureText(prompt, 20) / 2), 180, 20, WHITE);
}

static void draw_pause(void) {
    const char *text = "PAUSED";
    DrawText(text, (SCREEN_WIDTH / 2) - (MeasureText(text, 25) / 2), 200, 25, WHITE);
}

static void draw_game_over(const RenderSnapshot *snapshot) {
    const char *text = snapshot->winner == 1 ? "YOU WIN" : "YOU LOSE";
    const char *prompt = "PRESS ENTER";

    DrawText(text, (SCREEN_WIDTH / 2) - (MeasureText(text, 30) / 2), 120, 30, WHITE);
    DrawText(prompt, (SCREEN_WIDTH / 2) - (MeasureText(prompt, 20) / 2), 210, 20, WHITE);
}

// LIFECYCLE
void render_init(void) {
}

void render_shutdown(void) {
}

// MAIN RENDER
void render_frame(
    const RenderSnapshot *previous,
    const RenderSnapshot *current,
    float alpha
) {
    (void)previous;
    (void)alpha;

    switch (current->mode) {
        case GAME_TITLE:
            draw_title();
            break;
        
        case GAME_PLAYING:
            /* TODO: Render your game world, entities, and HUD here */
            break;
        
        case GAME_PAUSED:
            /* TODO: Render your game world, entities, and HUD here */
            draw_pause();
            break;
        
        case GAME_OVER:
            draw_game_over(current);
            break;
    }
}
