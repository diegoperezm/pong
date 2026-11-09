#include "render.h"
#include "raylib.h"
#include "config.h"

#define TILE_SIZE 90
#define TILE_GAP  6
#define BOARD_OFFSET_X ((SCREEN_WIDTH - (BOARD_SIZE * (TILE_SIZE + TILE_GAP) - TILE_GAP)) / 2)
#define BOARD_OFFSET_Y ((SCREEN_HEIGHT - (BOARD_SIZE * (TILE_SIZE + TILE_GAP) - TILE_GAP)) / 2 + 20)

void render_init(void) {}
void render_shutdown(void) {}

static float lerp(float a, float b, float t) {
    return a + (b - a) * t;
}

static void draw_title(void) {
    const char *title = "15-PUZZLE PHYSICS";
    const char *prompt = "PRESS ENTER OR SPACE";
    const char *instr = "Use Arrow Keys to Slide Tiles";

    DrawText(title, (SCREEN_WIDTH / 2) - (MeasureText(title, 36) / 2), 140, 36, RAYWHITE);
    DrawText(instr, (SCREEN_WIDTH / 2) - (MeasureText(instr, 20) / 2), 210, 20, LIGHTGRAY);
    DrawText(prompt, (SCREEN_WIDTH / 2) - (MeasureText(prompt, 20) / 2), 260, 20, YELLOW);
}

static void draw_board(const RenderSnapshot *previous, const RenderSnapshot *current, float alpha) {
    int board_w = BOARD_SIZE * (TILE_SIZE + TILE_GAP) - TILE_GAP;
    DrawRectangle(BOARD_OFFSET_X - 12, BOARD_OFFSET_Y - 12, board_w + 24, board_w + 24, DARKGRAY);
    DrawRectangleLines(BOARD_OFFSET_X - 12, BOARD_OFFSET_Y - 12, board_w + 24, board_w + 24, GRAY);

    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            int x = BOARD_OFFSET_X + c * (TILE_SIZE + TILE_GAP);
            int y = BOARD_OFFSET_Y + r * (TILE_SIZE + TILE_GAP);
            DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, Fade(BLACK, 0.4f));
        }
    }

    for (int i = 0; i < TOTAL_TILES - 1; ++i) {
        uint8_t val = current->tiles[i].value;
        if (val == 0) continue;

        float row = lerp(previous->tiles[i].row, current->tiles[i].row, alpha);
        float col = lerp(previous->tiles[i].col, current->tiles[i].col, alpha);

        int x = BOARD_OFFSET_X + (int)(col * (float)(TILE_SIZE + TILE_GAP));
        int y = BOARD_OFFSET_Y + (int)(row * (float)(TILE_SIZE + TILE_GAP));

        DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, LIGHTGRAY);
        DrawRectangleLines(x, y, TILE_SIZE, TILE_SIZE, DARKGRAY);

        const char *text = TextFormat("%d", val);
        int font_size = 32;
        int text_w = MeasureText(text, font_size);
        DrawText(text, x + (TILE_SIZE - text_w) / 2, y + (TILE_SIZE - font_size) / 2, font_size, BLACK);
    }

    DrawText(TextFormat("Moves: %u", current->move_count), BOARD_OFFSET_X, BOARD_OFFSET_Y - 40, 22, RAYWHITE);
    DrawText("P: Pause | F3: Debug", SCREEN_WIDTH - 180, BOARD_OFFSET_Y - 40, 16, LIGHTGRAY);
}

static void draw_pause(void) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.7f));
    const char *text = "PAUSED";
    DrawText(text, (SCREEN_WIDTH / 2) - (MeasureText(text, 30) / 2), SCREEN_HEIGHT / 2 - 20, 30, RAYWHITE);
}

static void draw_game_over(const RenderSnapshot *current) {
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.75f));
    const char *text = "PUZZLE SOLVED!";
    const char *score_text = TextFormat("Total Moves: %u", current->move_count);
    const char *prompt = "PRESS ENTER TO PLAY AGAIN";

    DrawText(text, (SCREEN_WIDTH / 2) - (MeasureText(text, 35) / 2), SCREEN_HEIGHT / 2 - 50, 35, GREEN);
    DrawText(score_text, (SCREEN_WIDTH / 2) - (MeasureText(score_text, 22) / 2), SCREEN_HEIGHT / 2, 22, RAYWHITE);
    DrawText(prompt, (SCREEN_WIDTH / 2) - (MeasureText(prompt, 20) / 2), SCREEN_HEIGHT / 2 + 50, 20, YELLOW);
}

void render_frame(const RenderSnapshot *previous, const RenderSnapshot *current, float alpha) {
    switch (current->mode) {
        case GAME_TITLE:
            draw_title();
            break;
        case GAME_PLAYING:
            draw_board(previous, current, alpha);
            break;
        case GAME_PAUSED:
            draw_board(previous, current, alpha);
            draw_pause();
            break;
        case GAME_OVER:
            draw_board(previous, current, alpha);
            draw_game_over(current);
            break;
    }
}
