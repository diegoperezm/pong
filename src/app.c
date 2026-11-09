#include "app.h"
#include "config.h"
#include "log.h"
#include "rng.h"
#include "simulation.h"

#include <stdio.h>
#include <string.h>

static Direction input_direction(const GameInput *input)
{
    if (input->up) return DIR_UP;
    if (input->down) return DIR_DOWN;
    if (input->left) return DIR_LEFT;
    if (input->right) return DIR_RIGHT;
    return DIR_NONE;
}

static void set_status(App *app, const char *message)
{
    if (!app || !message) return;
    (void)snprintf(app->status, sizeof(app->status), "%s", message);
}

static void start_match(App *app)
{
    uint64_t seed = rng_splitmix64(app->base_seed + app->match_counter++);
    event_queue_clear(&app->events);
    animation_reset(&app->animation);
    sim_begin_match(&app->sim, seed, &app->events);
    app->mode = GAME_PLAYING;
    (void)snprintf(app->status, sizeof(app->status), "Puzzle started (seed %016llx)",
                   (unsigned long long)seed);
    LOG_GAME("match started; seed=%016llx", (unsigned long long)seed);
}

void app_init(App *app, uint64_t base_seed)
{
    if (!app) return;
    memset(app, 0, sizeof(*app));
    app->mode = GAME_TITLE;
    app->base_seed = base_seed;
    event_queue_clear(&app->events);
    animation_reset(&app->animation);
    set_status(app, "Press Enter or Space to start");
}

void app_shutdown(App *app)
{
    if (!app) return;
    sim_shutdown(&app->sim);
    animation_reset(&app->animation);
}

void app_frame(App *app, const GameInput *input, float frame_time)
{
    Direction  direction;
    MoveResult move;

    if (!app || !input) return;

    if ((app->mode == GAME_TITLE || app->mode == GAME_OVER) && input->start) {
        start_match(app);
        return;
    }

    if (app->mode == GAME_PLAYING && input->pause) {
        app->mode = GAME_PAUSED;
        set_status(app, "Paused");
        return;
    }

    if (app->mode == GAME_PAUSED) {
        if (input->pause) {
            app->mode = GAME_PLAYING;
            set_status(app, "Resumed");
        }
        return;
    }

    if (app->mode != GAME_PLAYING) return;

    if (frame_time < 0.0f) frame_time = 0.0f;
    if (frame_time > MAX_FRAME_TIME) frame_time = MAX_FRAME_TIME;

    animation_update(&app->animation, frame_time);

    /* The simulation accepts discrete moves immediately. The application
       serializes input while the visual transition is in progress. */
    if (app->animation.active) return;

    if (app->sim.match_over) {
        app->mode = GAME_OVER;
        set_status(app, "Puzzle solved");
        LOG_GAME("match completed in %u moves", app->sim.move_count);
        return;
    }

    direction = input_direction(input);
    if (direction == DIR_NONE) return;

    if (sim_move(&app->sim, direction, &move, &app->events)) {
        animation_start(&app->animation, &move, TILE_ANIMATION_DURATION);
        (void)snprintf(app->status, sizeof(app->status), "Moved tile %u", move.tile_value);
        LOG_SIMULATION("tile=%u from=%u to=%u moves=%u",
                       move.tile_value, move.from_index, move.to_index,
                       app->sim.move_count);
    }
}

AppView app_view(const App *app)
{
    AppView view = {0};
    if (!app) return view;
    view.mode = app->mode;
    view.sim = &app->sim;
    view.animation = &app->animation;
    view.status = app->status;
    return view;
}
