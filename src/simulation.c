#include "simulation.h"
#include "log.h"
#include <string.h>

/* Bump when the rules of your game change in a way that alters a match, 
 * so old replays are refused instead of silently diverging. */
#define SIM_LOGIC_VERSION    1u
#define SIM_SUBSTEPS         4

/* Everything a system needs for one tick. */
typedef struct {
    SimulationState *s;
    const TickInput *input;
    float            dt;
    EventQueue      *events;
} StepCtx;

static void emit(const StepCtx *c, SimEvent *e) {
    e->tick = c->s->tick;
    event_queue_push(c->events, e);
}

/* ============================================================
 * GAME SYSTEMS (Implement your game logic here)
 * ============================================================ */

static void game_systems(StepCtx *c) {
    /* TODO: Add your game-specific systems here (e.g., player movement, 
     * AI updates, custom entity logic, collision/rule checks). */
    (void)c;
}

/* ============================================================
 * PUBLIC API
 * ============================================================ */

void sim_shutdown(SimulationState *state) {
    if (b2World_IsValid(state->world)) {
        b2DestroyWorld(state->world);   /* destroys every body in it */
    }
    state->world = b2_nullWorldId;
    
    /* TODO: Clean up any custom entity pools or state here */
}

void sim_begin_match(SimulationState *state, uint64_t seed, EventQueue *events) {
    sim_shutdown(state);

    /* Wipe EVERYTHING. A match is a pure function of (seed, inputs). */
    memset(state, 0, sizeof(*state));

    state->seed = seed;
    rng_seed(&state->rng, seed, 1);

    /* Initialize physics world */
    b2WorldDef world_def = b2DefaultWorldDef();
    world_def.gravity    = (b2Vec2){0.0f, 0.0f}; /* Change gravity if needed */
    state->world         = b2CreateWorld(&world_def);

    LOG_SIMULATION("match begin: seed=%llu", (unsigned long long)seed);

    StepCtx ctx = { .s = state, .input = NULL, .dt = 0.0f, .events = events };

    SimEvent e = { .type = EVT_MATCH_STARTED };
    emit(&ctx, &e);

    /* TODO: Spawn initial game entities / setup starting state here */
}

void sim_step(SimulationState *state, const TickInput *input, float dt, EventQueue *events) {
    if (state->match_over || !b2World_IsValid(state->world)) return;

    StepCtx ctx = { .s = state, .input = input, .dt = dt, .events = events };

    /* 1. Run game logic & input handling systems */
    game_systems(&ctx);

    /* 2. Advance physics world */
    b2World_Step(state->world, dt, SIM_SUBSTEPS);

    /* TODO: Sync physics body positions back to your custom game state structs */

    state->tick++;
}

