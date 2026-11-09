#include "render.h"
#include "config.h"
#include "log.h"
#include "raylib.h"

#define TILE_SIZE 88
#define TILE_GAP 8
#define BOARD_PIXEL_SIZE (BOARD_SIZE * (TILE_SIZE + TILE_GAP) - TILE_GAP)
#define BOARD_OFFSET_X ((SCREEN_WIDTH - BOARD_PIXEL_SIZE) / 2)
#define BOARD_OFFSET_Y ((SCREEN_HEIGHT - BOARD_PIXEL_SIZE) / 2 + 18)

static const Color COLOR_BACKGROUND = { 18, 20, 27, 255 };
static const Color COLOR_PANEL = { 32, 36, 47, 255 };
static const Color COLOR_TILE = { 225, 229, 235, 255 };
static const Color COLOR_TILE_EDGE = { 150, 160, 175, 255 };
static const Color COLOR_TEXT = { 235, 238, 244, 255 };

static float lerp(float a, float b, float t)
{
    return a + (b - a) * t;
}

static void draw_centered(const char *text, int y, int font_size, Color color)
{
    int width = MeasureText(text, font_size);
    DrawText(text, (SCREEN_WIDTH - width) / 2, y, font_size, color);
}

static void draw_title(void)
{
    draw_centered("15-PUZZLE", 165, 48, COLOR_TEXT);
    draw_centered("DISCRETE LOGIC / VISUAL ANIMATION", 226, 18, LIGHTGRAY);
    draw_centered("PRESS ENTER OR SPACE", 310, 22, YELLOW);
    draw_centered("Arrow keys or WASD to slide tiles", 356, 18, COLOR_TEXT);
}

static void draw_tile(uint8_t value, float row, float col)
{
    int x = BOARD_OFFSET_X + (int)(col * (float)(TILE_SIZE + TILE_GAP));
    int y = BOARD_OFFSET_Y + (int)(row * (float)(TILE_SIZE + TILE_GAP));
    const char *label = TextFormat("%u", (unsigned)value);
    int font_size = 30;
    int text_width = MeasureText(label, font_size);

    DrawRectangle(x + 2, y + 3, TILE_SIZE, TILE_SIZE, Fade(BLACK, 0.28f));
    DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, COLOR_TILE);
    DrawRectangleLines(x, y, TILE_SIZE, TILE_SIZE, COLOR_TILE_EDGE);
    DrawText(label, x + (TILE_SIZE - text_width) / 2,
             y + (TILE_SIZE - font_size) / 2, font_size, COLOR_PANEL);
}

static void draw_board(const AppView *view)
{
    const SimulationState *sim = view->sim;
    const TileAnimation *animation = view->animation;
    int i;

    DrawRectangle(BOARD_OFFSET_X - 14, BOARD_OFFSET_Y - 14,
                  BOARD_PIXEL_SIZE + 28, BOARD_PIXEL_SIZE + 28, COLOR_PANEL);
    DrawRectangleLines(BOARD_OFFSET_X - 14, BOARD_OFFSET_Y - 14,
                       BOARD_PIXEL_SIZE + 28, BOARD_PIXEL_SIZE + 28, GRAY);

    for (i = 0; i < TOTAL_TILES; ++i) {
        int row = i / BOARD_SIZE;
        int col = i % BOARD_SIZE;
        int x = BOARD_OFFSET_X + col * (TILE_SIZE + TILE_GAP);
        int y = BOARD_OFFSET_Y + row * (TILE_SIZE + TILE_GAP);
        DrawRectangle(x, y, TILE_SIZE, TILE_SIZE, Fade(BLACK, 0.35f));
    }

    for (i = 0; i < TOTAL_TILES; ++i) {
        uint8_t value = sim->board[i];
        if (value == 0) continue;
        if (animation && animation->active && value == animation->tile_value) continue;
        draw_tile(value, (float)(i / BOARD_SIZE), (float)(i % BOARD_SIZE));
    }

    if (animation && animation->active) {
        float t = animation_progress(animation);
        float from_row = (float)(animation->from_index / BOARD_SIZE);
        float from_col = (float)(animation->from_index % BOARD_SIZE);
        float to_row = (float)(animation->to_index / BOARD_SIZE);
        float to_col = (float)(animation->to_index % BOARD_SIZE);
        draw_tile(animation->tile_value,
                  lerp(from_row, to_row, t),
                  lerp(from_col, to_col, t));
    }

    DrawText(TextFormat("MOVES  %u", sim->move_count), BOARD_OFFSET_X,
             BOARD_OFFSET_Y - 43, 20, COLOR_TEXT);
    DrawText("ARROWS / WASD: MOVE     P / ESC: PAUSE     F3: DEBUG",
             BOARD_OFFSET_X, BOARD_OFFSET_Y + BOARD_PIXEL_SIZE + 29, 14, LIGHTGRAY);
}

static void draw_pause(void)
{
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.68f));
    draw_centered("PAUSED", SCREEN_HEIGHT / 2 - 16, 34, COLOR_TEXT);
    draw_centered("PRESS P OR ESC TO RESUME", SCREEN_HEIGHT / 2 + 28, 17, YELLOW);
}

static void draw_game_over(const AppView *view)
{
    DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, Fade(BLACK, 0.72f));
    draw_centered("PUZZLE SOLVED!", SCREEN_HEIGHT / 2 - 54, 38, GREEN);
    draw_centered(TextFormat("TOTAL MOVES: %u", view->sim->move_count),
                  SCREEN_HEIGHT / 2 + 2, 22, COLOR_TEXT);
    draw_centered("PRESS ENTER TO PLAY AGAIN", SCREEN_HEIGHT / 2 + 52, 18, YELLOW);
}

void render_frame(const AppView *view)
{
    if (!view) return;
    ClearBackground(COLOR_BACKGROUND);

    switch (view->mode) {
        case GAME_TITLE:
            draw_title();
            break;
        case GAME_PLAYING:
            draw_board(view);
            break;
        case GAME_PAUSED:
            draw_board(view);
            draw_pause();
            break;
        case GAME_OVER:
            draw_board(view);
            draw_game_over(view);
            break;
        default:
            draw_centered("INVALID GAME STATE", SCREEN_HEIGHT / 2, 24, RED);
            break;
    }
}
