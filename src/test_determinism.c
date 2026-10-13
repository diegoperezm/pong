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
#include "events.h"
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
static void run_headless(uint64_t seed, uint32_t salt, uint32_t stop_at, RunResult *r)
{
    memset(r, 0, sizeof *r);
    r->event_hash = hash_begin();
    r->fx_hash    = hash_begin();

    SimulationState *s  = calloc(1, sizeof *s);
    EventQueue      *q  = calloc(1, sizeof *q);
    Fx              *fx = calloc(1, sizeof *fx);

    InputSource src;
    input_source_use_live(&src);

    event_queue_clear(q);
    fx_reset(fx, seed, q);
    sim_begin_match(s, seed, q);
    fx_consume_events(fx, q);

    EventCursor ec = { 0, 0 };
    drain(q, &ec, r);

    while (!s->match_over && s->ticks < MAX_TICKS && s->tick < stop_at) {
        uint32_t tick = s->tick;
        TickInput in = script_input(tick, salt); // Re-enabled script inputs!

        sim_step(s, &in, (float)SIM_DT, q);
        fx_consume_events(fx, q);
        fx_update(fx, (float)SIM_DT);
        drain(q, &ec, r);
        r->ticks++;
    }

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        r->fx_hash = hash_u32(r->fx_hash, (uint32_t)fx->particles.active[i]);
        if (fx->particles.active[i]) {
            r->fx_hash = hash_f32(r->fx_hash, fx->particles.x[i]);
            r->fx_hash = hash_f32(r->fx_hash, fx->particles.y[i]);
        }
    }

    r->match_over = s->match_over;
    r->winner     = s->winner;
    r->score_p    = s->player.score;
    r->score_e    = s->enemy.score;

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

static void test_same_seed_same_match(void)
{
    printf("[1] same seed + same inputs => identical match\n");
    RunResult a, b;
    run_headless(12345, 7, UINT32_MAX, &a);
    run_headless(12345, 7, UINT32_MAX, &b);

    CHECK(a.match_over, "match did not finish within %u ticks", MAX_TICKS);
    CHECK(a.ticks > 0, "no ticks ran");
    CHECK(a.event_hash == b.event_hash, "event streams differ");
    CHECK(a.score_p == b.score_p && a.score_e == b.score_e, "final scores differ");
    run_free(&a); run_free(&b);
}


static void test_inputs_and_seed_matter(void) {
    printf("[2] a different seed or different inputs => a different match\n");
    RunResult a, b, c;
    run_headless(12345, 7,  3000, &a);
    run_headless(54321, 7,  3000, &b);   /* other seed  */
    run_headless(12345, 8,  3000, &c);   /* other input */

    CHECK(!traces_equal(&a, &b), "a different seed produced an identical trace");
    CHECK(!traces_equal(&a, &c), "different input produced an identical trace");
    run_free(&a); run_free(&b); run_free(&c);
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


static void test_two_matches_are_independent(void) {
    printf("[9] a match does not depend on any earlier match (no hidden state)\n");
    /* Run seed B alone, then run A followed by B in one process: B must be
     * identical. This is what the full per-match reset is for. */
    RunResult b_alone, a, b_after;
    run_headless(777, 2,  UINT32_MAX, &b_alone);
    run_headless(111, 9,  UINT32_MAX, &a);
    run_headless(777, 2,  UINT32_MAX, &b_after);

    CHECK(traces_equal(&b_alone, &b_after), "match B differs when it follows match A");
    CHECK(b_alone.event_hash == b_after.event_hash, "match B events differ after match A");
    run_free(&b_alone); run_free(&a); run_free(&b_after);
}

int main(void) {
    printf("config hash: %016llx\n\n", (unsigned long long)sim_config_hash());

    test_same_seed_same_match();
    test_inputs_and_seed_matter();
    test_event_ring();
    test_pool_reset_and_serials();
    test_two_matches_are_independent();

    printf("\n%d checks, %d failed\n", g_checks, g_failures);
    return g_failures ? 1 : 0;
}
