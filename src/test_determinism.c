/* Headless tests for the deterministic core. No window, no raylib.
 *
 *   make fake    -> runs against tests/fake_box2d (validates the PLUMBING)
 *   make real    -> runs against real Box2D       (validates Box2D determinism)
 *
 * The test source is identical in both cases. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "app.h"
#include "simulation.h"
#include "checksum.h"
#include "events.h"
#include "replay.h"
#include "input_source.h"
#include "fx.h"
#include "entities.h"
#include "hash.h"

static int g_checks = 0, g_failures = 0;

#define CHECK(cond, ...) do { \
    g_checks++; \
    if (!(cond)) { g_failures++; printf("    FAIL %s:%d: ", __FILE__, __LINE__); \
                   printf(__VA_ARGS__); printf("\n"); } \
} while (0)

#define TMP_REPLAY "/tmp/pong_test.replay"
#define MAX_TICKS  300000u

/* ------------------------------------------------------------------ */
/* Deterministic scripted "player" (independent of the sim's RNG)      */
/* ------------------------------------------------------------------ */

static TickInput script_input(uint32_t tick, uint32_t salt) {
    uint32_t h = (tick / 40u + salt) * 2654435761u;
    uint32_t phase = (h >> 13) % 4u;
    return (TickInput){ phase == 0u, phase == 1u };
}

/* ------------------------------------------------------------------ */
/* Headless runner: same per-tick pipeline as app.c                    */
/* ------------------------------------------------------------------ */

typedef struct {
    uint64_t *trace;          /* checksum after each tick */
    uint32_t  ticks;
    uint64_t  event_hash;     /* hash of EVERY event, in order */
    uint64_t  fx_hash;
    uint64_t  final_checksum;
    uint32_t  event_count;
    uint32_t  paddle_hits, wall_hits, scores;
    bool      match_over;
    int       winner, score_p, score_e;
} RunResult;

static uint64_t hash_event(uint64_t h, const SimEvent *e) {
    h = hash_u32(h, e->tick);
    h = hash_u32(h, (uint32_t)e->type);
    h = hash_u32(h, e->ball_serial);
    h = hash_f32(h, e->x);  h = hash_f32(h, e->y);
    h = hash_f32(h, e->vx); h = hash_f32(h, e->vy);
    h = hash_u32(h, (uint32_t)e->side);
    h = hash_u32(h, (uint32_t)e->wall);
    h = hash_u32(h, (uint32_t)e->score_player);
    return hash_u32(h, (uint32_t)e->score_enemy);
}

static void drain(const EventQueue *q, EventCursor *c, RunResult *r) {
    SimEvent e;
    while (event_cursor_read(q, c, &e)) {
        r->event_hash = hash_event(r->event_hash, &e);
        r->event_count++;
        if (e.type == EVT_BALL_HIT_PADDLE) r->paddle_hits++;
        if (e.type == EVT_BALL_HIT_WALL)   r->wall_hits++;
        if (e.type == EVT_POINT_SCORED)    r->scores++;
    }
}

/* replay_in != NULL: inputs come from it; otherwise from script_input(salt).
 * record_out != NULL: record the run the way the app does.
 * stop_at: stop after this many ticks (UINT32_MAX = run to the end). */
static void run_headless(uint64_t seed, uint32_t salt, const ReplayData *replay_in,
                         ReplayData *record_out, uint32_t stop_at, RunResult *r)
{
    memset(r, 0, sizeof *r);
    r->trace      = malloc(sizeof(uint64_t) * (MAX_TICKS + 1));
    r->event_hash = hash_begin();
    r->fx_hash    = hash_begin();

    SimulationState *s  = calloc(1, sizeof *s);
    EventQueue      *q  = calloc(1, sizeof *q);
    Fx              *fx = calloc(1, sizeof *fx);

    InputSource src;
    input_source_use_live(&src);
    if (replay_in) input_source_use_replay(&src, replay_in);

    event_queue_clear(q);
    fx_reset(fx, seed, q);
    sim_begin_match(s, seed, q);
    fx_consume_events(fx, q);

    EventCursor ec = { 0, 0 };
    drain(q, &ec, r);

    if (record_out) replay_begin(record_out, seed, sim_config_hash());

    while (!s->match_over && s->tick < MAX_TICKS && s->tick < stop_at) {
        uint32_t tick = s->tick;

        TickInput in = replay_in ? input_source_at(&src, tick) : script_input(tick, salt);

        if (record_out) {
            if (tick % REPLAY_CHECKPOINT_INTERVAL == 0u)
                replay_record_checkpoint(record_out, tick, sim_checksum(s));
            replay_record_tick(record_out, tick, in);
        }

        sim_step(s, &in, (float)SIM_DT, q);
        fx_consume_events(fx, q);
        fx_update(fx, (float)SIM_DT);
        drain(q, &ec, r);

        r->trace[r->ticks++] = sim_checksum(s);
    }

    if (record_out) replay_finish(record_out, s->tick, sim_checksum(s));

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        r->fx_hash = hash_u32(r->fx_hash, (uint32_t)fx->particles.active[i]);
        if (fx->particles.active[i]) {
            r->fx_hash = hash_f32(r->fx_hash, fx->particles.x[i]);
            r->fx_hash = hash_f32(r->fx_hash, fx->particles.y[i]);
        }
    }

    r->final_checksum = sim_checksum(s);
    r->match_over = s->match_over;
    r->winner  = s->winner;
    r->score_p = s->player.score;
    r->score_e = s->enemy.score;

    sim_shutdown(s);
    free(s); free(q); free(fx);
}

static void run_free(RunResult *r) { free(r->trace); r->trace = NULL; }

static bool traces_equal(const RunResult *a, const RunResult *b) {
    return a->ticks == b->ticks &&
           memcmp(a->trace, b->trace, sizeof(uint64_t) * a->ticks) == 0;
}

/* ------------------------------------------------------------------ */
/* Tests                                                               */
/* ------------------------------------------------------------------ */

static void test_same_seed_same_match(void) {
    printf("[1] same seed + same inputs => identical match\n");
    RunResult a, b;
    run_headless(12345, 7, NULL, NULL, UINT32_MAX, &a);
    run_headless(12345, 7, NULL, NULL, UINT32_MAX, &b);

    printf("    match: %u ticks, score %d-%d, %u events (%u paddle, %u wall, %u scores)\n",
           a.ticks, a.score_p, a.score_e, a.event_count, a.paddle_hits, a.wall_hits, a.scores);

    CHECK(a.match_over, "match did not finish within %u ticks", MAX_TICKS);
    CHECK(a.ticks > 0, "no ticks ran");
    CHECK(a.paddle_hits > 0, "no paddle-hit events were produced");
    CHECK(a.scores > 0, "no score events were produced");
    CHECK(traces_equal(&a, &b), "per-tick checksum traces differ");
    CHECK(a.event_hash == b.event_hash, "event streams differ");
    CHECK(a.fx_hash == b.fx_hash, "particle state differs");
    CHECK(a.final_checksum == b.final_checksum, "final checksum differs");
    run_free(&a); run_free(&b);
}

static void test_inputs_and_seed_matter(void) {
    printf("[2] a different seed or different inputs => a different match\n");
    RunResult a, b, c;
    run_headless(12345, 7, NULL, NULL, 3000, &a);
    run_headless(54321, 7, NULL, NULL, 3000, &b);   /* other seed  */
    run_headless(12345, 8, NULL, NULL, 3000, &c);   /* other input */

    CHECK(!traces_equal(&a, &b), "a different seed produced an identical trace");
    CHECK(!traces_equal(&a, &c), "different input produced an identical trace");
    run_free(&a); run_free(&b); run_free(&c);
}

static void test_record_save_load_replay(void) {
    printf("[3] record -> save -> load -> replay reproduces the match exactly\n");
    RunResult live, replayed;
    ReplayData rec, loaded;
    replay_init(&rec); replay_init(&loaded);

    run_headless(2024, 3, NULL, &rec, UINT32_MAX, &live);
    CHECK(rec.complete, "recording not marked complete");
    CHECK(rec.tick_count == live.ticks, "recorded tick_count %u != %u", rec.tick_count, live.ticks);
    CHECK(replay_save(&rec, TMP_REPLAY), "save failed");
    CHECK(replay_load(&loaded, TMP_REPLAY), "load failed");

    CHECK(loaded.seed == rec.seed && loaded.tick_count == rec.tick_count &&
          loaded.change_count == rec.change_count &&
          loaded.checkpoint_count == rec.checkpoint_count &&
          loaded.final_checksum == rec.final_checksum &&
          loaded.config_hash == rec.config_hash, "loaded header/body differs from saved");

    run_headless(loaded.seed, 0, &loaded, NULL, UINT32_MAX, &replayed);
    CHECK(traces_equal(&live, &replayed), "replayed trace differs from the live match");
    CHECK(live.event_hash == replayed.event_hash, "replayed events differ");
    CHECK(replayed.final_checksum == loaded.final_checksum, "final checksum mismatch");

    /* checkpoint at tick T is the state BEFORE tick T == trace[T-1] */
    uint64_t cp;
    CHECK(replay_checkpoint_for(&loaded, 0, &cp), "no checkpoint at tick 0");
    if (live.ticks > REPLAY_CHECKPOINT_INTERVAL) {
        CHECK(replay_checkpoint_for(&loaded, REPLAY_CHECKPOINT_INTERVAL, &cp) &&
              cp == live.trace[REPLAY_CHECKPOINT_INTERVAL - 1],
              "checkpoint at tick %u does not match the trace", REPLAY_CHECKPOINT_INTERVAL);
    }

    printf("    replay: %u ticks, %u input changes, %u checkpoints\n",
           loaded.tick_count, loaded.change_count, loaded.checkpoint_count);

    run_free(&live); run_free(&replayed);
    replay_free(&rec); replay_free(&loaded);
}

static void test_seek_by_resimulation(void) {
    printf("[4] seeking = re-simulating from tick 0 lands on the same state\n");
    RunResult live, seek;
    ReplayData rec;
    replay_init(&rec);
    run_headless(99, 5, NULL, &rec, UINT32_MAX, &live);

    const uint32_t targets[] = { 1, 119, 120, 777, live.ticks / 2, live.ticks - 1 };
    for (size_t i = 0; i < sizeof targets / sizeof targets[0]; ++i) {
        uint32_t t = targets[i];
        if (t == 0 || t > live.ticks) continue;
        run_headless(rec.seed, 0, &rec, NULL, t, &seek);
        CHECK(seek.ticks == t, "seek to %u ran %u ticks", t, seek.ticks);
        CHECK(seek.final_checksum == live.trace[t - 1],
              "seek to tick %u: state differs from the straight run", t);
        run_free(&seek);
    }
    run_free(&live);
    replay_free(&rec);
}

static void test_corrupt_replays_rejected(void) {
    printf("[5] corrupt or truncated replay files are rejected safely\n");
    ReplayData rec, keep, probe;
    RunResult r;
    replay_init(&rec); replay_init(&keep); replay_init(&probe);

    run_headless(7, 1, NULL, &rec, UINT32_MAX, &r);
    run_free(&r);
    CHECK(replay_save(&rec, TMP_REPLAY), "save failed");
    CHECK(replay_load(&keep, TMP_REPLAY), "load failed");

    /* read the good file */
    FILE *f = fopen(TMP_REPLAY, "rb");
    fseek(f, 0, SEEK_END); long size = ftell(f); fseek(f, 0, SEEK_SET);
    unsigned char *buf = malloc((size_t)size);
    CHECK(fread(buf, 1, (size_t)size, f) == (size_t)size, "read back failed");
    fclose(f);

    const char *bad = "/tmp/pong_test_bad.replay";
    struct { const char *name; long len; int flip_at; } cases[] = {
        { "truncated to half",      size / 2, -1 },
        { "truncated to header",    20,       -1 },
        { "bad magic",              size,      0 },
        { "bad version",            size,      8 },
    };
    for (size_t i = 0; i < sizeof cases / sizeof cases[0]; ++i) {
        unsigned char *copy = malloc((size_t)size);
        memcpy(copy, buf, (size_t)size);
        if (cases[i].flip_at >= 0) copy[cases[i].flip_at] ^= 0xFF;
        f = fopen(bad, "wb"); fwrite(copy, 1, (size_t)cases[i].len, f); fclose(f);
        free(copy);

        bool ok = replay_load(&keep, bad);
        CHECK(!ok, "'%s' was accepted", cases[i].name);
        /* a failed load must leave existing data untouched */
        CHECK(keep.seed == rec.seed && keep.tick_count == rec.tick_count,
              "failed load ('%s') damaged the existing replay", cases[i].name);
    }
    CHECK(!replay_load(&probe, "/tmp/definitely/not/here.replay"), "missing file accepted");

    free(buf);
    replay_free(&rec); replay_free(&keep); replay_free(&probe);
}

static void test_event_ring(void) {
    printf("[6] event ring: cursors, overwrite-oldest, clear\n");
    EventQueue *q = calloc(1, sizeof *q);
    EventCursor slow = {0, 0}, fast = {0, 0};

    for (uint32_t i = 0; i < 300; ++i) {
        SimEvent e = { .tick = i, .type = EVT_BALL_HIT_WALL };
        event_queue_push(q, &e);
        SimEvent out;
        CHECK(event_cursor_read(q, &fast, &out) && out.tick == i, "fast consumer missed event %u", i);
    }
    CHECK(fast.lost == 0, "fast consumer reports %u lost", fast.lost);

    SimEvent out;
    uint32_t read = 0, first_tick = 0;
    while (event_cursor_read(q, &slow, &out)) { if (read == 0) first_tick = out.tick; read++; }
    CHECK(read == EVENT_RING_CAPACITY, "slow consumer read %u, expected %u", read, EVENT_RING_CAPACITY);
    CHECK(slow.lost == 300u - EVENT_RING_CAPACITY, "slow consumer lost %u, expected %u",
          slow.lost, 300u - EVENT_RING_CAPACITY);
    CHECK(first_tick == 300u - EVENT_RING_CAPACITY, "slow consumer resumed at tick %u", first_tick);

    CHECK(event_queue_recent(q, 0, &out) && out.tick == 299, "newest event wrong");
    CHECK(event_queue_recent(q, 255, &out) && out.tick == 44, "oldest retained event wrong");
    CHECK(!event_queue_recent(q, 256, &out), "recent() reached past the ring");

    event_queue_clear(q);   /* cursors are now ahead of head: must self-heal */
    CHECK(!event_cursor_read(q, &slow, &out), "stale cursor read from an empty queue");
    SimEvent e = { .tick = 1000, .type = EVT_POINT_SCORED };
    event_queue_push(q, &e);
    CHECK(event_cursor_read(q, &slow, &out) && out.tick == 1000, "cursor did not recover after clear");

    free(q);
}

static void test_pool_reset_and_serials(void) {
    printf("[7] ball pool: serials never reused, per-match reset is complete\n");
    b2WorldDef wd = b2DefaultWorldDef();
    b2WorldId w = b2CreateWorld(&wd);
    BallPool *p = calloc(1, sizeof *p);
    ball_pool_init(p);

    EntityHandle h1 = ball_create(p, w, 100, 100, 10, 10);
    EntityHandle h2 = ball_create(p, w, 200, 100, 10, 10);
    CHECK(ball_is_valid(p, h1) && ball_is_valid(p, h2), "balls not created");
    CHECK(p->serial[p->slots[h1.index].dense_idx] == 1, "first serial != 1");
    CHECK(p->serial[p->slots[h2.index].dense_idx] == 2, "second serial != 2");

    ball_destroy(p, h1);
    CHECK(!ball_is_valid(p, h1), "stale handle still valid after destroy");
    CHECK(ball_is_valid(p, h2), "survivor invalidated by swap-remove");
    CHECK(p->serial[p->slots[h2.index].dense_idx] == 2, "serial lost in swap-remove");

    EntityHandle h3 = ball_create(p, w, 300, 100, 10, 10);
    CHECK(h3.index == h1.index, "freed slot was not reused");
    CHECK(h3.generation == h1.generation + 1, "generation not bumped on reuse");
    CHECK(!ball_is_valid(p, h1), "stale handle revalidated by slot reuse (ABA)");
    CHECK(p->serial[p->slots[h3.index].dense_idx] == 3, "serial reused");

    uint32_t order[MAX_BALLS];
    uint32_t n = balls_sorted_by_serial(p, order);
    CHECK(n == 2 && p->serial[order[0]] < p->serial[order[1]], "serial order wrong");

    balls_clear(p);   /* full reset */
    CHECK(p->count == 0 && p->next_serial == 1, "reset left count/serial behind");
    bool all_gen1 = true;
    for (uint32_t i = 0; i < MAX_BALLS; ++i) if (p->slots[i].generation != 1) all_gen1 = false;
    CHECK(all_gen1, "generations were not reset");
    CHECK(!ball_is_valid(p, h2) && !ball_is_valid(p, h3), "old handles survived the reset");

    free(p);
    b2DestroyWorld(w);
}

/* ---- App layer, end to end ------------------------------------------- */

static GameInput frame_input(const App *app, uint32_t salt) {
    TickInput t = script_input(app->sim.tick, salt);
    GameInput g;
    memset(&g, 0, sizeof g);
    g.up = t.up; g.down = t.down;
    return g;
}

static bool run_frames_until_over(App *app, uint32_t salt, uint32_t max_frames) {
    for (uint32_t f = 0; f < max_frames && app->mode != GAME_OVER; ++f) {
        GameInput g = frame_input(app, salt);
        app_frame(app, &g, 1.0 / 60.0);
    }
    return app->mode == GAME_OVER;
}

static void test_app_end_to_end(void) {
    printf("[8] app: play -> auto-save -> watch replay (verified) -> pause/step/seek\n");
    App *app = calloc(1, sizeof *app);
    app_init(app, 4242);
    snprintf(app->replay_path, sizeof app->replay_path, "%s", TMP_REPLAY);
    remove(TMP_REPLAY);

    CHECK(app->mode == GAME_TITLE, "does not start on the title screen");

    GameInput g; memset(&g, 0, sizeof g);

    /* F6 with nothing recorded must be refused */
    g.replay = true; app_frame(app, &g, 1.0 / 60.0);
    CHECK(app->mode == GAME_TITLE && !app->replaying, "replay started with no file");
    CHECK(strstr(app->status, "no valid replay") != NULL, "status: '%s'", app->status);

    /* play a live match to the end */
    memset(&g, 0, sizeof g); g.start = true; app_frame(app, &g, 1.0 / 60.0);
    CHECK(app->mode == GAME_PLAYING && !app->replaying, "start did not begin a live match");
    CHECK(run_frames_until_over(app, 11, 400000), "live match never ended");

    uint32_t live_ticks = app->sim.tick;
    uint64_t live_final = app->last_checksum;
    int live_winner = app->sim.winner;
    printf("    live match: %u ticks, winner=%d, %s\n", live_ticks, live_winner, app->status);
    CHECK(strstr(app->status, "saved replay") != NULL, "match end did not save: '%s'", app->status);

    /* watch it back */
    memset(&g, 0, sizeof g); g.replay = true; app_frame(app, &g, 1.0 / 60.0);
    CHECK(app->mode == GAME_PLAYING && app->replaying, "F6 did not start the replay");
    CHECK(run_frames_until_over(app, 0, 400000), "replay never ended");
    CHECK(app->replay_result == REPLAY_RESULT_OK, "replay not verified (result=%d): '%s'",
          app->replay_result, app->status);
    CHECK(app->sim.tick == live_ticks, "replay ran %u ticks, live ran %u", app->sim.tick, live_ticks);
    CHECK(app->last_checksum == live_final, "replay final state differs from live");
    CHECK(app->sim.winner == live_winner, "replay winner differs");

    /* a replay from a different sim version must be refused */
    {
        FILE *f = fopen(TMP_REPLAY, "r+b");
        CHECK(f != NULL, "cannot reopen replay file");
        if (f) {
            fseek(f, 16, SEEK_SET);          /* config_hash */
            int c = fgetc(f);
            fseek(f, 16, SEEK_SET);
            fputc(c ^ 0xFF, f);
            fclose(f);
        }
        memset(&g, 0, sizeof g); g.replay = true; app_frame(app, &g, 1.0 / 60.0);
        CHECK(app->mode == GAME_OVER && !app->replaying, "mismatched replay was started");
        CHECK(strstr(app->status, "different sim version") != NULL, "status: '%s'", app->status);
        CHECK(replay_save(&app->recording, TMP_REPLAY), "could not restore the replay file");
    }

    /* pause / step / seek inside a replay */
    memset(&g, 0, sizeof g); g.replay = true; app_frame(app, &g, 1.0 / 60.0);
    CHECK(app->replaying && app->mode == GAME_PLAYING, "second replay did not start");
    for (int i = 0; i < 400; ++i) { memset(&g, 0, sizeof g); app_frame(app, &g, 1.0 / 60.0); }
    uint32_t t0 = app->sim.tick;
    CHECK(t0 > 600, "replay did not advance (tick %u)", t0);

    memset(&g, 0, sizeof g); g.pause = true; app_frame(app, &g, 1.0 / 60.0);
    CHECK(app->mode == GAME_PAUSED, "pause ignored");
    for (int i = 0; i < 100; ++i) { memset(&g, 0, sizeof g); app_frame(app, &g, 1.0 / 60.0); }
    CHECK(app->sim.tick == t0, "ticks advanced while paused (%u -> %u)", t0, app->sim.tick);

    memset(&g, 0, sizeof g); g.step = true; app_frame(app, &g, 1.0 / 60.0);
    CHECK(app->sim.tick == t0 + 1, "single-step moved %u ticks", app->sim.tick - t0);

    uint32_t t1 = app->sim.tick;
    memset(&g, 0, sizeof g); g.seek_fwd = true; app_frame(app, &g, 1.0 / 60.0);
    CHECK(app->sim.tick == t1 + SIM_HZ, "seek forward: %u -> %u", t1, app->sim.tick);
    CHECK(app->mode == GAME_PAUSED && app->replaying, "seek changed mode");

    RunResult ref;
    run_headless(app->loaded.seed, 0, &app->loaded, NULL, app->sim.tick, &ref);
    CHECK(app->last_checksum == ref.final_checksum, "state after seek differs from a straight run");
    run_free(&ref);

    uint32_t t2 = app->sim.tick;
    memset(&g, 0, sizeof g); g.seek_back = true; app_frame(app, &g, 1.0 / 60.0);
    CHECK(app->sim.tick == t2 - SIM_HZ, "seek back: %u -> %u", t2, app->sim.tick);
    run_headless(app->loaded.seed, 0, &app->loaded, NULL, app->sim.tick, &ref);
    CHECK(app->last_checksum == ref.final_checksum, "state after seeking BACK differs from a straight run");
    run_free(&ref);

    CHECK(app->replay_result != REPLAY_RESULT_DESYNC, "desync flagged during seek/step");

    /* resume and finish: still verified */
    memset(&g, 0, sizeof g); g.pause = true; app_frame(app, &g, 1.0 / 60.0);
    CHECK(app->mode == GAME_PLAYING, "resume failed");
    CHECK(run_frames_until_over(app, 0, 400000), "resumed replay never ended");
    CHECK(app->replay_result == REPLAY_RESULT_OK, "resumed replay not verified: '%s'", app->status);

    /* a new live match after all that starts clean */
    memset(&g, 0, sizeof g); g.start = true; app_frame(app, &g, 1.0 / 60.0);
    CHECK(app->mode == GAME_PLAYING && !app->replaying, "could not start a new match");
    /* The start frame also runs that frame's ticks (about 2 at 60 fps), so
     * the tick counter is small but not necessarily 0. */
    CHECK(app->sim.tick <= 3 && app->sim.player.score == 0 && app->sim.enemy.score == 0,
          "new match did not start from a clean state (tick %u)", app->sim.tick);
    CHECK(app->replay_result == REPLAY_RESULT_NONE, "old replay result leaked into the new match");

    app_shutdown(app);
    free(app);
}

static void test_two_matches_are_independent(void) {
    printf("[9] a match does not depend on any earlier match (no hidden state)\n");
    /* Run seed B alone, then run A followed by B in one process: B must be
     * identical. This is what the full per-match reset is for. */
    RunResult b_alone, a, b_after;
    run_headless(777, 2, NULL, NULL, UINT32_MAX, &b_alone);
    run_headless(111, 9, NULL, NULL, UINT32_MAX, &a);
    run_headless(777, 2, NULL, NULL, UINT32_MAX, &b_after);

    CHECK(traces_equal(&b_alone, &b_after), "match B differs when it follows match A");
    CHECK(b_alone.event_hash == b_after.event_hash, "match B events differ after match A");
    run_free(&b_alone); run_free(&a); run_free(&b_after);
}

int main(void) {
    printf("config hash: %016llx\n\n", (unsigned long long)sim_config_hash());

    test_same_seed_same_match();
    test_inputs_and_seed_matter();
    test_record_save_load_replay();
    test_seek_by_resimulation();
    test_corrupt_replays_rejected();
    test_event_ring();
    test_pool_reset_and_serials();
    test_app_end_to_end();
    test_two_matches_are_independent();

    printf("\n%d checks, %d failed\n", g_checks, g_failures);
    return g_failures ? 1 : 0;
}
