#include "debug.h"
#include "config.h"
#include "render.h"
#include "raylib.h"

#include <stdarg.h>
#include <stdio.h>

#define DEBUG_FPS_SMOOTHING   0.10f  /* EMA weight of the newest sample   */
#define DEBUG_VELOCITY_SCALE  0.5f   /* seconds of travel the line shows  */

#define HIST_FONT_SIZE        12
#define HIST_LINE_HEIGHT      15
#define HIST_EVENT_LINES      8
#define HIST_INPUT_LINES      4

/* ============================================================
 * INIT / UPDATE
 * ============================================================ */

void debug_init(DebugState *debug) {
    debug->enabled         = 0;
    debug->show_collisions = 0;
    debug->show_velocity   = 0;
    debug->frame_time      = 0.0f;
    debug->fps             = 0.0f;
    debug->ball_count      = 0;
    debug->particle_count  = 0;
    debug->powerup_count   = 0;
}

static int debug_count_particles(const RenderSnapshot *snapshot) {
    int count = 0;
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (snapshot->particles[i].active)
            ++count;
    }
    return count;
}

static int debug_count_powerups(const RenderSnapshot *snapshot) {
    int count = 0;
    for (int i = 0; i < MAX_POWERUPS; ++i) {
        if (snapshot->powerups[i].active)
            ++count;
    }
    return count;
}

void debug_update(DebugState *debug, const RenderSnapshot *current, float raw_frame_time) {
    if (debug == NULL || current == NULL || !debug->enabled) return;

    debug->frame_time = raw_frame_time;

    if (raw_frame_time > 0.0f) {
        float instant_fps = 1.0f / raw_frame_time;
        debug->fps = (debug->fps <= 0.0f)
            ? instant_fps
            : debug->fps + (instant_fps - debug->fps) * DEBUG_FPS_SMOOTHING;
    }

    debug->ball_count     = current->ball_count;
    debug->particle_count = debug_count_particles(current);
    debug->powerup_count  = debug_count_powerups(current);
}

/* ============================================================
 * TEXT HELPERS
 * ============================================================ */

typedef struct {
    int x;
    int y;
} DebugCursor;

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 5, 6)))
#endif
static void text_line(DebugCursor *cursor, int font, int line_height, Color color,
                      const char *fmt, ...) {
    char text[160];

    va_list args;
    va_start(args, fmt);
    vsnprintf(text, sizeof(text), fmt, args);
    va_end(args);

    DrawText(text, cursor->x, cursor->y, font, color);
    cursor->y += line_height;
}

static const char *debug_game_mode_name(GameMode mode) {
    switch (mode) {
        case GAME_TITLE:   return "TITLE";
        case GAME_PLAYING: return "PLAYING";
        case GAME_PAUSED:  return "PAUSED";
        case GAME_OVER:    return "GAME OVER";
        default:           return "UNKNOWN";
    }
}

/* ============================================================
 * PANEL 1: live numbers (from the snapshot being drawn)
 * ============================================================ */

static void debug_draw_panel(const DebugState *debug, const RenderSnapshot *current) {
    DrawRectangle(DEBUG_PANEL_X, DEBUG_PANEL_Y, DEBUG_PANEL_WIDTH, DEBUG_PANEL_HEIGHT, Fade(BLACK, 0.80f));
    DrawRectangleLines(DEBUG_PANEL_X, DEBUG_PANEL_Y, DEBUG_PANEL_WIDTH, DEBUG_PANEL_HEIGHT, RAYWHITE);

    DebugCursor c = { DEBUG_PANEL_X + DEBUG_PADDING, DEBUG_PANEL_Y + DEBUG_PADDING };
    const int F = DEBUG_FONT_SIZE, L = DEBUG_LINE_HEIGHT;

    text_line(&c, F, L, RAYWHITE, "DEBUG");
    text_line(&c, F, L, RAYWHITE, "FPS: %.1f", debug->fps);
    text_line(&c, F, L, RAYWHITE, "Frame: %.3f ms", debug->frame_time * 1000.0f);
    text_line(&c, F, L, RAYWHITE, "Mode: %s", debug_game_mode_name(current->mode));
    text_line(&c, F, L, RAYWHITE, "Time: %.2f", current->game_time);
    text_line(&c, F, L, RAYWHITE, "Player: (%.1f, %.1f)  score %d",
              current->player_x, current->player_y, current->player_score);
    text_line(&c, F, L, RAYWHITE, "Enemy:  (%.1f, %.1f)  score %d",
              current->enemy_x, current->enemy_y, current->enemy_score);
    text_line(&c, F, L, RAYWHITE, "Balls: %d / %d", debug->ball_count, MAX_BALLS);
    text_line(&c, F, L, RAYWHITE, "Particles: %d / %d", debug->particle_count, MAX_PARTICLES);
    text_line(&c, F, L, RAYWHITE, "Powerups: %d / %d", debug->powerup_count, MAX_POWERUPS);
}

/* ============================================================
 * PANEL 2: history (event ring, input log, replay status)
 * ============================================================ */

static const char *side_name(Side s) { return s == SIDE_PLAYER ? "player" : "enemy"; }

static void event_detail(const SimEvent *e, char *buf, size_t n) {
    switch (e->type) {
        case EVT_BALL_HIT_PADDLE:
            snprintf(buf, n, "%s paddle", side_name(e->side));
            break;
        case EVT_BALL_HIT_WALL:
            snprintf(buf, n, "%s wall", e->wall == WALL_TOP ? "top" : "bottom");
            break;
        case EVT_POINT_SCORED:
            snprintf(buf, n, "%s scores  %d-%d", side_name(e->side), e->score_player, e->score_enemy);
            break;
        case EVT_MATCH_ENDED:
            snprintf(buf, n, "%s wins  %d-%d", side_name(e->side), e->score_player, e->score_enemy);
            break;
        default:
            buf[0] = '\0';
            break;
    }
}

static void debug_draw_history(const DebugHistory *h) {
    const int x = DEBUG_PANEL_X + DEBUG_PANEL_WIDTH + 10;
    const int y = DEBUG_PANEL_Y;
    const int w = SCREEN_WIDTH - x - 10;
    const int F = HIST_FONT_SIZE, L = HIST_LINE_HEIGHT;

    DrawRectangle(x, y, w, DEBUG_PANEL_HEIGHT, Fade(BLACK, 0.80f));
    DrawRectangleLines(x, y, w, DEBUG_PANEL_HEIGHT, RAYWHITE);

    DebugCursor c = { x + DEBUG_PADDING, y + DEBUG_PADDING };

    text_line(&c, F, L, RAYWHITE, "SIM   tick %u   seed %016llx",
              h->tick, (unsigned long long)h->seed);
    text_line(&c, F, L, RAYWHITE, "state checksum %016llx",
              (unsigned long long)h->checksum);

    /* replay status */
    if (h->replaying) {
        text_line(&c, F, L, YELLOW, "REPLAY  %u / %u ticks", h->tick, h->replay_total_ticks);
    } else {
        text_line(&c, F, L, RAYWHITE, "live");
    }
    if (h->replay_result == REPLAY_RESULT_DESYNC)
        text_line(&c, F, L, RED, "DESYNC at tick %u", h->desync_tick);
    else if (h->replay_result == REPLAY_RESULT_OK)
        text_line(&c, F, L, GREEN, "replay verified: every checksum matched");
    else
        text_line(&c, F, L, RAYWHITE, " ");

    /* event history, oldest first */
    text_line(&c, F, L, RAYWHITE, "EVENTS (last %d)   lost: %u", HIST_EVENT_LINES, h->events_lost);
    uint64_t total = event_queue_total(h->events);
    uint32_t avail = total < HIST_EVENT_LINES ? (uint32_t)total : HIST_EVENT_LINES;
    for (int i = HIST_EVENT_LINES - 1; i >= 0; --i) {
        SimEvent e;
        if ((uint32_t)i >= avail || !event_queue_recent(h->events, (uint32_t)i, &e)) {
            text_line(&c, F, L, RAYWHITE, " ");
            continue;
        }
        char detail[64];
        event_detail(&e, detail, sizeof detail);
        text_line(&c, F, L, RAYWHITE, "%6u  %-11s #%u  %s",
                  e.tick, event_type_name(e.type), e.ball_serial, detail);
    }

    /* input timeline: last few CHANGES in the stream being recorded/played */
    text_line(&c, F, L, RAYWHITE, "INPUT CHANGES (last %d)", HIST_INPUT_LINES);
    const ReplayData *log = h->input_log;
    uint32_t count = log ? log->change_count : 0;
    uint32_t first = count > HIST_INPUT_LINES ? count - HIST_INPUT_LINES : 0;
    for (int i = 0; i < HIST_INPUT_LINES; ++i) {
        uint32_t idx = first + (uint32_t)i;
        if (idx < count)
            text_line(&c, F, L, RAYWHITE, "%6u  up=%d down=%d",
                      log->changes[idx].tick, log->changes[idx].up, log->changes[idx].down);
        else
            text_line(&c, F, L, RAYWHITE, " ");
    }

    text_line(&c, F, L, YELLOW, "%s", (h->status && h->status[0]) ? h->status : " ");
    text_line(&c, F, L, RAYWHITE, "F3 debug  F6 replay  P pause  . step  [ ] seek");
}

/* ============================================================
 * WORLD OVERLAYS (snapshot-based, interpolated like the sprites)
 * ============================================================ */

static bool debug_mode_has_world(GameMode mode) {
    return mode == GAME_PLAYING || mode == GAME_PAUSED;
}

static void debug_ball_center(RenderVec2 pos, int *cx, int *cy) {
    *cx = (int)pos.x + (int)(BALL_SIZE * 0.5f);
    *cy = (int)pos.y + (int)(BALL_SIZE * 0.5f);
}

static void debug_draw_collisions(
    const RenderSnapshot *previous,
    const RenderSnapshot *current,
    float alpha)
{
    RenderVec2 player = render_player_position(previous, current, alpha);
    RenderVec2 enemy  = render_enemy_position(previous, current, alpha);

    DrawRectangleLines((int)player.x, (int)player.y,
                       (int)PADDLE_WIDTH, (int)PADDLE_HEIGHT, RED);
    DrawRectangleLines((int)enemy.x, (int)enemy.y,
                       (int)PADDLE_WIDTH, (int)PADDLE_HEIGHT, RED);

    for (int i = 0; i < current->ball_count; ++i) {
        RenderVec2 pos;
        if (!render_ball_position(previous, current, i, alpha, &pos))
            continue;

        int cx, cy;
        debug_ball_center(pos, &cx, &cy);
        DrawCircleLines(cx, cy, BALL_SIZE * 0.5f, GREEN);
    }
}

static void debug_draw_velocity(
    const RenderSnapshot *previous,
    const RenderSnapshot *current,
    float alpha)
{
    for (int i = 0; i < current->ball_count; ++i) {
        RenderVec2 pos;
        if (!render_ball_position(previous, current, i, alpha, &pos))
            continue;

        int cx, cy;
        debug_ball_center(pos, &cx, &cy);

        const RenderBall *ball = &current->balls[i];
        int ex = cx + (int)(ball->vx * DEBUG_VELOCITY_SCALE);
        int ey = cy + (int)(ball->vy * DEBUG_VELOCITY_SCALE);

        DrawLine(cx, cy, ex, ey, YELLOW);
    }
}

/* ============================================================
 * ENTRY POINT
 * ============================================================ */

void debug_draw(
    const DebugState *debug,
    const RenderSnapshot *previous,
    const RenderSnapshot *current,
    float alpha,
    const DebugHistory *history)
{
    if (debug == NULL || previous == NULL || current == NULL)
        return;
    if (!debug->enabled)
        return;

    if (debug_mode_has_world(current->mode)) {
        if (debug->show_collisions)
            debug_draw_collisions(previous, current, alpha);
        if (debug->show_velocity)
            debug_draw_velocity(previous, current, alpha);
    }

    debug_draw_panel(debug, current);
    if (history)
        debug_draw_history(history);
}
