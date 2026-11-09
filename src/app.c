#include "app.h"
#include "simulation.h"
#include "rng.h"
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 2, 3)))
#endif
static void set_status(App *app, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(app->status, sizeof app->status, fmt, args);
    va_end(args);
}

static void make_snapshot(const App *app, RenderSnapshot *snap) {
    const SimulationState *s = &app->sim;
    snap->mode = app->mode;
    snap->move_count = s->move_count;
    snap->game_time = (float)s->tick * SIM_DT;
    snap->is_solved = s->is_solved;

    for (int i = 0; i < TOTAL_TILES - 1; ++i) {
        snap->tiles[i].value = s->tiles[i].value;
        snap->tiles[i].row   = s->tiles[i].position.row;
        snap->tiles[i].col   = s->tiles[i].position.col;
    }
}

static void snapshots_reset(App *app) {
    make_snapshot(app, &app->current);
    app->previous = app->current;
}

static void begin_match_common(App *app, uint64_t seed) {
    event_queue_clear(&app->events);
    sim_begin_match(&app->sim, seed, &app->events);
    app->accumulator = 0.0;
}

static void start_live_match(App *app) {
    uint64_t seed = rng_splitmix64(app->base_seed + app->match_counter++);
    input_source_use_live(&app->source);
    begin_match_common(app, seed);
    app->mode = GAME_PLAYING;
    snapshots_reset(app);
    set_status(app, "puzzle started (seed %016llx)", (unsigned long long)seed);
}

static void run_tick(App *app, bool snapshot) {
    SimulationState *sim = &app->sim;
    const uint32_t tick = sim->tick;
    TickInput in = input_source_at(&app->source, tick);

    if (snapshot) app->previous = app->current;
    sim_step(sim, &in, (float)SIM_DT, &app->events);
    input_source_consume_one_shot(&app->source);

    if (sim->match_over) {
        app->mode = GAME_OVER;
        app->accumulator = 0.0;
        // Fix Critical 1: Update snapshot after changing mode to GAME_OVER
        if (snapshot) {
            make_snapshot(app, &app->current);
            app->previous = app->current;
        }
    } else {
        if (snapshot) make_snapshot(app, &app->current);
    }
}

static void handle_input(App *app, const GameInput *in) {
    switch (app->mode) {
        case GAME_TITLE:
        case GAME_OVER:
            if (in->start) start_live_match(app);
            break;
        case GAME_PLAYING:
            if (in->pause) {
                app->mode = GAME_PAUSED;
                input_source_clear(&app->source); // Fix Medium 7: clear pending input on pause
            }
            break;
        case GAME_PAUSED:
            if (in->pause) {
                app->mode = GAME_PLAYING;
                input_source_clear(&app->source);
            }
            break;
    }
}

void app_init(App *app, uint64_t base_seed) {
    memset(app, 0, sizeof(*app));
    app->mode = GAME_TITLE;
    app->base_seed = base_seed;
    app->alpha = 1.0f;
    input_source_use_live(&app->source);
    event_queue_clear(&app->events);
    snapshots_reset(app);
}

void app_shutdown(App *app) {
    sim_shutdown(&app->sim);
}

void app_frame(App *app, const GameInput *input, double frame_time) {
    GameMode before = app->mode;
    handle_input(app, input);

    if (app->mode != before) {
        app->accumulator = 0.0;
        snapshots_reset(app);
    }

    if (app->mode == GAME_PLAYING) {
        Direction dir = DIR_NONE;
        if (input->up)    dir = DIR_UP;
        if (input->down)  dir = DIR_DOWN;
        if (input->left)  dir = DIR_LEFT;
        if (input->right) dir = DIR_RIGHT;

        if (dir != DIR_NONE) {
            input_source_set_pending(&app->source, (TickInput){ .move_dir = dir });
        }

        app->accumulator += frame_time;

        while (app->accumulator >= (double)SIM_DT) {
            run_tick(app, true);
            app->accumulator -= (double)SIM_DT;

            if (app->mode != GAME_PLAYING) {
                app->accumulator = 0.0;
                break;
            }
        }

        if (app->mode == GAME_PLAYING) {
            app->alpha = (float)(app->accumulator / (double)SIM_DT);
        } else {
            app->alpha = 1.0f;
        }
    } else {
        app->alpha = 1.0f;
    }
}

AppView app_view(const App *app) {
    return (AppView){ &app->previous, &app->current, app->alpha };
}
