#include "app.h"
#include "checksum.h"
#include "rng.h"
#include "simulation.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#define SEEK_STEP_TICKS ((uint32_t)SIM_HZ)   /* one second */

#if defined(__GNUC__) || defined(__clang__)
__attribute__((format(printf, 2, 3)))
#endif
static void set_status(App *app, const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vsnprintf(app->status, sizeof app->status, fmt, args);
    va_end(args);
}

/* ============================================================
 * SNAPSHOTS
 * ============================================================ */

static void make_snapshot(const App *app, RenderSnapshot *snap) {
    const SimulationState *s = &app->sim;

    snap->player_x     = s->player.x;
    snap->player_y     = s->player.y;
    snap->player_score = s->player.score;
    snap->enemy_x      = s->enemy.x;
    snap->enemy_y      = s->enemy.y;
    snap->enemy_score  = s->enemy.score;

    snap->mode      = app->mode;
    snap->winner    = s->winner;
    snap->tick      = s->tick;
    snap->game_time = (float)s->tick * SIM_DT;
    snap->replaying = app->replaying;

    int n = (int)s->balls.count;
    if (n > MAX_BALLS) n = MAX_BALLS;
    snap->ball_count = n;
    for (int d = 0; d < n; ++d) {
        snap->balls[d] = (RenderBall){
            .x = s->balls.x[d], .y = s->balls.y[d],
            .vx = s->balls.vx[d], .vy = s->balls.vy[d],
            .serial = s->balls.serial[d]
        };
    }

    const ParticlePool *p = &app->fx.particles;
    for (int i = 0; i < MAX_PARTICLES; ++i) {
        snap->particles[i].active       = p->active[i] != 0;
        snap->particles[i].x            = p->x[i];
        snap->particles[i].y            = p->y[i];
        snap->particles[i].lifetime     = p->lifetime[i];
        snap->particles[i].max_lifetime = p->max_lifetime[i];
        snap->particles[i].size         = p->size[i];
    }

    for (int i = 0; i < MAX_POWERUPS; ++i) {
        snap->powerups[i].active = s->powerups.active[i] != 0;
        snap->powerups[i].x      = s->powerups.x[i];
        snap->powerups[i].y      = s->powerups.y[i];
        snap->powerups[i].type   = s->powerups.type[i];
    }
}

/* Make both snapshots show the current state, so the next frame does not
 * interpolate from a stale one. */
static void snapshots_reset(App *app) {
    make_snapshot(app, &app->current);
    app->previous = app->current;
}

/* ============================================================
 * MATCH START / FINISH
 * ============================================================ */

/* Order matters: clear the queue, point fx at it, THEN let the sim emit. */
static void begin_match_common(App *app, uint64_t seed) {
    event_queue_clear(&app->events);
    fx_reset(&app->fx, seed, &app->events);
    sim_begin_match(&app->sim, seed, &app->events);
    fx_consume_events(&app->fx, &app->events);

    app->accumulator   = 0.0;
    app->last_checksum = sim_checksum(&app->sim);
    app->replay_result = REPLAY_RESULT_NONE;
    app->desync_tick   = 0;
}

static void start_live_match(App *app) {
    uint64_t seed = rng_splitmix64(app->base_seed + app->match_counter++);

    app->replaying = false;
    input_source_use_live(&app->source);
    replay_begin(&app->recording, seed, sim_config_hash());

    begin_match_common(app, seed);

    app->mode = GAME_PLAYING;
    snapshots_reset(app);
    set_status(app, "recording match (seed %016llx)", (unsigned long long)seed);
}

static void finish_live_match(App *app) {
    replay_finish(&app->recording, app->sim.tick, app->last_checksum);

    if (app->recording.failed) {
        set_status(app, "recording failed (out of memory): replay not saved");
    } else if (replay_save(&app->recording, app->replay_path)) {
        set_status(app, "saved replay: %u ticks, %u input changes. F6 to watch",
                   app->recording.tick_count, app->recording.change_count);
    } else {
        set_status(app, "could not write %s", app->replay_path);
    }
}

static void finish_replay_playback(App *app) {
    if (app->replay_result != REPLAY_RESULT_DESYNC) {
        bool same = app->sim.tick == app->loaded.tick_count &&
                    app->last_checksum == app->loaded.final_checksum;
        if (same) {
            app->replay_result = REPLAY_RESULT_OK;
            set_status(app, "replay verified (%u checkpoints + final state)",
                       app->loaded.checkpoint_count);
        } else {
            app->replay_result = REPLAY_RESULT_DESYNC;
            app->desync_tick   = app->sim.tick;
            set_status(app, "DESYNC: final state differs from the recording");
        }
    }
}

static void finish_match(App *app) {
    if (app->replaying) finish_replay_playback(app);
    else                finish_live_match(app);

    app->mode        = GAME_OVER;
    app->replaying   = false;
    app->accumulator = 0.0;
}

/* ============================================================
 * ONE TICK
 * ============================================================ */

static void verify_checkpoint(App *app, uint32_t tick, uint64_t checksum) {
    if (app->replay_result == REPLAY_RESULT_DESYNC) return;

    uint64_t expected;
    if (!replay_checkpoint_for(&app->loaded, tick, &expected)) return;

    if (expected != checksum) {
        app->replay_result = REPLAY_RESULT_DESYNC;
        app->desync_tick   = tick;
        set_status(app, "DESYNC at tick %u: expected %016llx, got %016llx",
                   tick, (unsigned long long)expected, (unsigned long long)checksum);
    }
}

/* The whole per-tick pipeline, identical for live play, replay and seek:
 *   checkpoint -> input for this tick -> sim_step -> consumers -> snapshot */
static void run_tick(App *app, bool snapshot) {
    SimulationState *sim = &app->sim;
    const uint32_t tick  = sim->tick;

    /* Checkpoints are taken BEFORE the tick runs, so a replay checks the
     * same moment it was recorded at. */
    if ((tick % REPLAY_CHECKPOINT_INTERVAL) == 0u) {
        uint64_t cs = sim_checksum(sim);
        if (app->replaying) verify_checkpoint(app, tick, cs);
        else                replay_record_checkpoint(&app->recording, tick, cs);
    }

    TickInput in = input_source_at(&app->source, tick);
    if (!app->replaying)
        replay_record_tick(&app->recording, tick, in);

    if (snapshot) app->previous = app->current;

    sim_step(sim, &in, (float)SIM_DT, &app->events);

    /* Consumers run after every tick, so effects stay tick-aligned. */
    fx_consume_events(&app->fx, &app->events);
    fx_update(&app->fx, (float)SIM_DT);

    app->last_checksum = sim_checksum(sim);
    if (snapshot) make_snapshot(app, &app->current);

    if (sim->match_over)
        finish_match(app);
    else if (app->replaying && sim->tick >= app->loaded.tick_count)
        finish_match(app);   /* recording exhausted */
}

/* ============================================================
 * REPLAY: START AND SEEK (seek = re-simulate from tick 0)
 * ============================================================ */

/* Re-runs the match from the beginning up to `target` ticks. Possible
 * because a match is a pure function of (seed, inputs). Does not touch
 * app->mode. */
static void replay_start_at(App *app, uint32_t target) {
    app->replaying = true;
    input_source_use_replay(&app->source, &app->loaded);
    begin_match_common(app, app->loaded.seed);

    while (app->replaying && !app->sim.match_over && app->sim.tick < target)
        run_tick(app, false);

    snapshots_reset(app);
}

static void start_replay(App *app) {
    if (!replay_load(&app->loaded, app->replay_path)) {
        set_status(app, "no valid replay at %s", app->replay_path);
        return;
    }
    if (app->loaded.config_hash != sim_config_hash() ||
        app->loaded.sim_hz != (uint32_t)SIM_HZ) {
        set_status(app, "replay is from a different sim version: refused");
        return;
    }
    if (app->loaded.tick_count == 0) {
        set_status(app, "replay is empty");
        return;
    }

    replay_start_at(app, 0);
    app->mode = GAME_PLAYING;
    snapshots_reset(app);
    set_status(app, "watching replay: %u ticks. [ ] seek, . step while paused",
               app->loaded.tick_count);
}

static void replay_seek(App *app, uint32_t target) {
    uint32_t total = app->loaded.tick_count;
    if (total == 0) return;
    if (target >= total) target = total - 1;

    replay_start_at(app, target);
    set_status(app, "seek: tick %u / %u", app->sim.tick, total);
}

/* ============================================================
 * APP-LEVEL INPUT (menus, pause, replay controls)
 * ============================================================ */

static void handle_input(App *app, const GameInput *in) {
    switch (app->mode) {
        case GAME_TITLE:
        case GAME_OVER:
            if (in->start)       start_live_match(app);
            else if (in->replay) start_replay(app);
            break;

        case GAME_PLAYING:
            if (in->pause) app->mode = GAME_PAUSED;
            break;

        case GAME_PAUSED:
            if (in->pause) app->mode = GAME_PLAYING;
            break;
    }

    if (app->replaying && (app->mode == GAME_PLAYING || app->mode == GAME_PAUSED)) {
        uint32_t t = app->sim.tick;
        if (in->seek_back)
            replay_seek(app, t > SEEK_STEP_TICKS ? t - SEEK_STEP_TICKS : 0u);
        else if (in->seek_fwd)
            replay_seek(app, t + SEEK_STEP_TICKS);
    }
}

/* ============================================================
 * PUBLIC API
 * ============================================================ */

void app_init(App *app, uint64_t base_seed) {
    memset(app, 0, sizeof *app);

    app->mode      = GAME_TITLE;
    app->base_seed = base_seed;
    app->alpha     = 1.0f;

    replay_init(&app->recording);
    replay_init(&app->loaded);
    input_source_use_live(&app->source);

    snprintf(app->replay_path, sizeof app->replay_path, "%s", APP_DEFAULT_REPLAY_PATH);

    event_queue_clear(&app->events);
    fx_reset(&app->fx, 0, &app->events);
    snapshots_reset(app);
}

void app_shutdown(App *app) {
    sim_shutdown(&app->sim);
    replay_free(&app->recording);
    replay_free(&app->loaded);
}

void app_frame(App *app, const GameInput *input, double frame_time) {
    GameMode before = app->mode;

    handle_input(app, input);

    if (app->mode != before) {
        app->accumulator = 0.0;
        snapshots_reset(app);
    }

    if (app->mode == GAME_PLAYING) {
        /* Held input is sampled once per frame; every tick of this frame
         * sees it. (Ignored when the source is a replay.) */
        input_source_set_live_held(&app->source,
                                   (TickInput){ input->up, input->down });

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

        /* Frame-step: only while paused inside a replay. */
        if (app->mode == GAME_PAUSED && app->replaying && input->step)
            run_tick(app, true);
    }
}

AppView app_view(const App *app) {
    return (AppView){ &app->previous, &app->current, app->alpha };
}

DebugHistory app_debug_history(const App *app) {
    DebugHistory h;
    memset(&h, 0, sizeof h);

    h.events             = &app->events;
    h.input_log          = app->replaying ? &app->loaded : &app->recording;
    h.tick               = app->sim.tick;
    h.seed               = app->sim.seed;
    h.checksum           = app->last_checksum;
    h.replaying          = app->replaying;
    h.replay_result      = app->replay_result;
    h.desync_tick        = app->desync_tick;
    h.replay_total_ticks = app->loaded.tick_count;
    h.events_lost        = app->fx.cursor.lost;
    h.status             = app->status;
    return h;
}
