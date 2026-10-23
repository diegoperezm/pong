#include "app.h"
#include "rng.h"
#include "simulation.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

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

    /* TODO: Copy game state values from s to snap for rendering/interpolation */
    snap->mode      = app->mode;
    snap->winner    = s->winner;
    snap->tick      = s->tick;
    snap->game_time = (float)s->tick * SIM_DT;
    (void)s;
}

static void snapshots_reset(App *app) {
    make_snapshot(app, &app->current);
    app->previous = app->current;
}

static void begin_match_common(App *app, uint64_t seed) {
    event_queue_clear(&app->events);
    fx_reset(&app->fx, seed, &app->events);
    sim_begin_match(&app->sim, seed, &app->events);
    fx_consume_events(&app->fx, &app->events);
    app->accumulator = 0.0;
}

static void start_live_match(App *app) {
    uint64_t seed = rng_splitmix64(app->base_seed + app->match_counter++);
    input_source_use_live(&app->source);
    begin_match_common(app, seed);

    app->mode = GAME_PLAYING;
    snapshots_reset(app);
    set_status(app, "match started (seed %016llx)", (unsigned long long)seed);
}

static void run_tick(App *app, bool snapshot) {
    SimulationState *sim = &app->sim;
    const uint32_t tick  = sim->tick;

    TickInput in = input_source_at(&app->source, tick);

    if (snapshot) app->previous = app->current;

    sim_step(sim, &in, (float)SIM_DT, &app->events);

    fx_consume_events(&app->fx, &app->events);
    fx_update(&app->fx, (float)SIM_DT);

    if (snapshot) make_snapshot(app, &app->current);

    if (sim->match_over) {
        app->mode = GAME_OVER;
        app->accumulator = 0.0;
    }
}

static void handle_input(App *app, const GameInput *in) {
    switch (app->mode) {
        case GAME_TITLE:
        case GAME_OVER:
            if (in->start) start_live_match(app);
            break;
        case GAME_PLAYING:
            if (in->pause) app->mode = GAME_PAUSED;
            break;
        case GAME_PAUSED:
            if (in->pause) app->mode = GAME_PLAYING;
            break;
    }
}

void app_init(App *app, uint64_t base_seed) {
    memset(app, 0, sizeof *app);
    app->mode      = GAME_TITLE;
    app->base_seed = base_seed;
    app->alpha     = 1.0f;

    input_source_use_live(&app->source);
    event_queue_clear(&app->events);
    fx_reset(&app->fx, 0, &app->events);
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
        input_source_set_live_held(&app->source, (TickInput){ input->up, input->down });
        app->accumulator += frame_time;

        while (app->mode == GAME_PLAYING && app->accumulator >= (double)SIM_DT) {
            run_tick(app, true);
            app->accumulator -= (double)SIM_DT;
        }

        if (app->mode != GAME_PLAYING) {
            app->accumulator = 0.0;
            snapshots_reset(app);
            app->alpha = 1.0f;
        } else {
            app->alpha = (float)(app->accumulator / (double)SIM_DT);
        }
    } else {
        app->alpha = 1.0f;
        if (app->mode == GAME_PAUSED && input->step)
            run_tick(app, true);
    }
}

AppView app_view(const App *app) {
    return (AppView){ &app->previous, &app->current, app->alpha };
}

DebugHistory app_debug_history(const App *app) {
    DebugHistory h;
    memset(&h, 0, sizeof h);
    h.events      = &app->events;
    h.tick        = app->sim.tick;
    h.seed        = app->sim.seed;
    h.events_lost = app->fx.cursor.lost;
    h.status      = app->status;
    return h;
}
