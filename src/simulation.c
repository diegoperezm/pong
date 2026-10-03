#include "simulation.h"
#include "entities.h"
#include "hash.h"
#include "log.h"
#include <math.h>
#include <string.h>

/* No raylib here on purpose: the simulation must not depend on the platform. */

/* Bump when the rules of the game change in a way that alters a match, so
 * old replays are refused instead of silently diverging. */
#define SIM_LOGIC_VERSION    1u

#define SIM_SUBSTEPS         4
#define SIM_PI               3.14159265358979f
#define PADDLE_INSET         20.0f
#define AI_DEADZONE_PX       6.0f
#define SERVE_ANGLE_MIN_DEG  20.0f
#define SERVE_ANGLE_MAX_DEG  40.0f

/* Everything a system needs for one tick. */
typedef struct {
    SimulationState *s;
    const TickInput *input;
    float            dt;
    EventQueue      *events;
} StepCtx;

static void emit(const StepCtx *c, SimEvent *e) {
    e->tick         = c->s->tick;
    e->score_player = c->s->player.score;
    e->score_enemy  = c->s->enemy.score;
    event_queue_push(c->events, e);
}

/* ============================================================
 * BODY CREATION
 * ============================================================ */

static b2BodyId create_static_box(b2WorldId world, float x, float y, float width, float height) {
    if (!b2World_IsValid(world)) return b2_nullBodyId;

    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type = b2_staticBody;
    body_def.position = pixel_to_meter(x + width * 0.5f, y + height * 0.5f);

    b2BodyId body = b2CreateBody(world, &body_def);
    if (!b2Body_IsValid(body)) return b2_nullBodyId;

    b2ShapeDef shape_def = b2DefaultShapeDef();
    shape_def.material.friction = 0.0f;
    shape_def.material.restitution = 1.0f;

    b2Polygon box = b2MakeBox(PX_TO_M(width * 0.5f), PX_TO_M(height * 0.5f));
    b2ShapeId shape_id = b2CreatePolygonShape(body, &shape_def, &box);

    if (!b2Shape_IsValid(shape_id)) {
        b2DestroyBody(body);
        return b2_nullBodyId;
    }

    return body;
}

static b2BodyId create_paddle_body(b2WorldId world, const Paddle *paddle) {
    if (!b2World_IsValid(world)) return b2_nullBodyId;

    if (paddle->width <= 0.0f || paddle->height <= 0.0f) {
        LOG_SIMULATION("Cannot create paddle body: invalid dimensions (w: %.1f, h: %.1f)",
                       paddle->width, paddle->height);
        return b2_nullBodyId;
    }

    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type = b2_kinematicBody;
    body_def.position = pixel_to_meter(paddle->x + paddle->width * 0.5f, paddle->y + paddle->height * 0.5f);

    b2BodyId body = b2CreateBody(world, &body_def);
    if (!b2Body_IsValid(body)) return b2_nullBodyId;

    b2ShapeDef shape_def = b2DefaultShapeDef();
    shape_def.material.friction = 0.0f;
    shape_def.material.restitution = 1.0f;

    b2Polygon box = b2MakeBox(PX_TO_M(paddle->width * 0.5f), PX_TO_M(paddle->height * 0.5f));
    b2ShapeId shape_id = b2CreatePolygonShape(body, &shape_def, &box);

    if (!b2Shape_IsValid(shape_id)) {
        b2DestroyBody(body);
        return b2_nullBodyId;
    }

    return body;
}

static void create_walls(b2WorldId world) {
    create_static_box(world, COURT_LEFT, COURT_TOP - 10.0f,
                      COURT_RIGHT - COURT_LEFT, 10.0f);
    create_static_box(world, COURT_LEFT, COURT_BOTTOM,
                      COURT_RIGHT - COURT_LEFT, 10.0f);
}

static void init_paddles(SimulationState *s) {
    float y = COURT_CENTER_Y - PADDLE_HEIGHT * 0.5f;

    s->player = (Paddle){
        .x = COURT_LEFT + PADDLE_INSET, .y = y,
        .width = PADDLE_WIDTH, .height = PADDLE_HEIGHT,
        .speed = PLAYER_SPEED, .score = 0
    };
    s->enemy = (Paddle){
        .x = COURT_RIGHT - PADDLE_WIDTH - PADDLE_INSET, .y = y,
        .width = PADDLE_WIDTH, .height = PADDLE_HEIGHT,
        .speed = AI_SPEED, .score = 0
    };
}

/* ============================================================
 * SYSTEMS (fixed order: players, step, sync, collisions, scoring)
 * ============================================================ */

static void sync_physics_to_state(SimulationState *state) {
    if (b2Body_IsValid(state->player_body)) {
        b2Vec2 pos = b2Body_GetPosition(state->player_body);
        state->player.y = M_TO_PX(pos.y) - state->player.height * 0.5f;
    }

    if (b2Body_IsValid(state->enemy_body)) {
        b2Vec2 pos = b2Body_GetPosition(state->enemy_body);
        state->enemy.y = M_TO_PX(pos.y) - state->enemy.height * 0.5f;
    }

    BallPool *balls = &state->balls;
    for (uint32_t i = 0; i < balls->count; ++i) {
        if (b2Body_IsValid(balls->body[i])) {
            b2Vec2 pos = b2Body_GetPosition(balls->body[i]);
            b2Vec2 vel = b2Body_GetLinearVelocity(balls->body[i]);

            balls->x[i]  = M_TO_PX(pos.x) - BALL_SIZE * 0.5f;
            balls->y[i]  = M_TO_PX(pos.y) - BALL_SIZE * 0.5f;
            balls->vx[i] = M_TO_PX(vel.x);
            balls->vy[i] = M_TO_PX(vel.y);
        }
    }
}

/* Predictive clamp: cap velocity so the paddle lands exactly on the court
 * bound after dt instead of overshooting. */
static void drive_paddle(b2BodyId body, const Paddle *p, float velocity, float dt) {
    if (!b2Body_IsValid(body)) return;

    if (dt > 0.0f) {
        float next_y = p->y + velocity * dt;
        float max_y  = COURT_BOTTOM - p->height;
        if (next_y < COURT_TOP)      velocity = (COURT_TOP - p->y) / dt;
        else if (next_y > max_y)     velocity = (max_y - p->y) / dt;
    }

    b2Body_SetLinearVelocity(body, (b2Vec2){0.0f, PX_TO_M(velocity)});
}

static void player_system(StepCtx *c) {
    SimulationState *s = c->s;

    float velocity = 0.0f;
    if (c->input->up)   velocity -= s->player.speed;
    if (c->input->down) velocity += s->player.speed;

    drive_paddle(s->player_body, &s->player, velocity, c->dt);
}

static void enemy_system(StepCtx *c) {
    SimulationState *s = c->s;
    BallPool *balls = &s->balls;

    /* Target: the ball furthest to the right. Ties go to the LOWER serial,
     * so the choice never depends on how the pool is laid out. */
    int      best = -1;
    float    best_x = 0.0f;
    uint32_t best_serial = 0;

    for (uint32_t d = 0; d < balls->count; ++d) {
        float    x = balls->x[d];
        uint32_t serial = balls->serial[d];
        if (best < 0 || x > best_x || (x == best_x && serial < best_serial)) {
            best = (int)d;
            best_x = x;
            best_serial = serial;
        }
    }

    float velocity = 0.0f;
    if (best >= 0) {
        float ball_y   = balls->y[best] + BALL_SIZE * 0.5f;
        float paddle_y = s->enemy.y + s->enemy.height * 0.5f;
        float diff     = ball_y - paddle_y;

        if (fabsf(diff) > AI_DEADZONE_PX)
            velocity = (diff < 0.0f) ? -s->enemy.speed : s->enemy.speed;
    }

    drive_paddle(s->enemy_body, &s->enemy, velocity, c->dt);
}

/* Paddle/wall hits are INFERRED from velocity sign flips across the physics
 * step (the paddle is a flat reflector on the x axis, the walls on the y
 * axis). x-flip => paddle hit, otherwise y-flip => wall hit. A ball skimming
 * a paddle's top or bottom edge therefore reports as a wall hit. */
static void collision_events(StepCtx *c, const float *prev_vx, const float *prev_vy) {
    BallPool *b = &c->s->balls;

    uint32_t order[MAX_BALLS];
    uint32_t n = balls_sorted_by_serial(b, order);

    for (uint32_t k = 0; k < n; ++k) {
        uint32_t d = order[k];
        float cx = b->x[d] + BALL_SIZE * 0.5f;
        float cy = b->y[d] + BALL_SIZE * 0.5f;

        bool flip_x = (prev_vx[d] * b->vx[d]) < 0.0f;
        bool flip_y = (prev_vy[d] * b->vy[d]) < 0.0f;

        if (flip_x) {
            SimEvent e = {
                .type = EVT_BALL_HIT_PADDLE, .ball_serial = b->serial[d],
                .x = cx, .y = cy, .vx = b->vx[d], .vy = b->vy[d],
                .side = (cx < COURT_CENTER_X) ? SIDE_PLAYER : SIDE_ENEMY
            };
            emit(c, &e);
        } else if (flip_y) {
            SimEvent e = {
                .type = EVT_BALL_HIT_WALL, .ball_serial = b->serial[d],
                .x = cx, .y = cy, .vx = b->vx[d], .vy = b->vy[d],
                .wall = (cy < COURT_CENTER_Y) ? WALL_TOP : WALL_BOTTOM
            };
            emit(c, &e);
        }
    }
}

static void serve_ball(StepCtx *c, float direction) {
    SimulationState *s = c->s;

    /* One RNG draw per statement: argument evaluation order is unspecified
     * in C, so draws are never nested inside a call. */
    float angle_deg = rng_float_range(&s->rng, SERVE_ANGLE_MIN_DEG, SERVE_ANGLE_MAX_DEG);
    float vsign     = (rng_next_u32(&s->rng) & 1u) ? 1.0f : -1.0f;

    float angle = angle_deg * (SIM_PI / 180.0f);
    float vx = cosf(angle) * INITIAL_BALL_SPEED * direction;
    float vy = sinf(angle) * INITIAL_BALL_SPEED * vsign;

    EntityHandle h = ball_create(&s->balls, s->world,
                                 COURT_CENTER_X - BALL_SIZE * 0.5f,
                                 COURT_CENTER_Y - BALL_SIZE * 0.5f,
                                 vx, vy);
    if (!ball_is_valid(&s->balls, h)) return;

    uint32_t dense = s->balls.slots[h.index].dense_idx;
    SimEvent e = {
        .type = EVT_BALL_SERVED, .ball_serial = s->balls.serial[dense],
        .x = COURT_CENTER_X, .y = COURT_CENTER_Y, .vx = vx, .vy = vy
    };
    emit(c, &e);
}

typedef struct {
    EntityHandle handle;
    uint32_t     serial;
    Side         scorer;
    float        x, y, vx, vy;
} ScoreHit;

static void scoring_system(StepCtx *c) {
    SimulationState *s = c->s;
    BallPool *b = &s->balls;

    /* Pass 1: find balls that left the court, in serial order. Dense
     * indices are still valid here (nothing has been destroyed yet). */
    ScoreHit hits[MAX_BALLS];
    uint32_t hit_count = 0;

    uint32_t order[MAX_BALLS];
    uint32_t n = balls_sorted_by_serial(b, order);

    for (uint32_t k = 0; k < n; ++k) {
        uint32_t d = order[k];
        float px = b->x[d];

        bool left  = px < COURT_LEFT - BALL_SIZE;
        bool right = px > COURT_RIGHT;
        if (!left && !right) continue;

        uint32_t slot = b->dense_to_sparse[d];
        hits[hit_count++] = (ScoreHit){
            .handle = { .index = slot, .generation = b->slots[slot].generation },
            .serial = b->serial[d],
            .scorer = left ? SIDE_ENEMY : SIDE_PLAYER,
            .x = px + BALL_SIZE * 0.5f,
            .y = b->y[d] + BALL_SIZE * 0.5f,
            .vx = b->vx[d],
            .vy = b->vy[d]
        };
    }

    /* Pass 2: apply them. Handles survive swap-remove; dense indices do not. */
    for (uint32_t i = 0; i < hit_count; ++i) {
        const ScoreHit *h = &hits[i];

        if (h->scorer == SIDE_PLAYER) { s->player.score++; s->last_scorer = 1; }
        else                          { s->enemy.score++;  s->last_scorer = 2; }

        SimEvent e = {
            .type = EVT_POINT_SCORED, .ball_serial = h->serial,
            .x = h->x, .y = h->y, .vx = h->vx, .vy = h->vy,
            .side = h->scorer
        };
        emit(c, &e);

        ball_destroy(b, h->handle);
    }

    int winner = 0;
    if (s->player.score >= WINNING_SCORE)     winner = 1;
    else if (s->enemy.score >= WINNING_SCORE) winner = 2;

    if (winner != 0) {
        s->match_over = true;
        s->winner = winner;

        SimEvent e = {
            .type = EVT_MATCH_ENDED,
            .side = (winner == 1) ? SIDE_PLAYER : SIDE_ENEMY
        };
        emit(c, &e);
        return;
    }

    if (b->count == 0) {
        /* Serve toward the side that just conceded. */
        serve_ball(c, (s->last_scorer == 1) ? 1.0f : -1.0f);
    }
}

/* ============================================================
 * PUBLIC API
 * ============================================================ */

void sim_shutdown(SimulationState *state) {
    if (b2World_IsValid(state->world)) {
        b2DestroyWorld(state->world);   /* destroys every body in it */
    }
    state->world       = b2_nullWorldId;
    state->player_body = b2_nullBodyId;
    state->enemy_body  = b2_nullBodyId;
    ball_pool_init(&state->balls);
}

void sim_begin_match(SimulationState *state, uint64_t seed, EventQueue *events) {
    sim_shutdown(state);

    /* Wipe EVERYTHING. A match is a pure function of (seed, inputs), so no
     * field may survive from the previous one. */
    memset(state, 0, sizeof(*state));

    state->seed = seed;
    rng_seed(&state->rng, seed, 1);

    init_paddles(state);
    ball_pool_init(&state->balls);
    powerups_clear(&state->powerups);

    b2WorldDef world_def = b2DefaultWorldDef();
    world_def.gravity    = (b2Vec2){0.0f, 0.0f};
    state->world         = b2CreateWorld(&world_def);

    create_walls(state->world);
    state->player_body = create_paddle_body(state->world, &state->player);
    state->enemy_body  = create_paddle_body(state->world, &state->enemy);

    LOG_SIMULATION("match begin: seed=%llu", (unsigned long long)seed);

    StepCtx ctx = { .s = state, .input = NULL, .dt = 0.0f, .events = events };

    SimEvent e = { .type = EVT_MATCH_STARTED };
    emit(&ctx, &e);

    serve_ball(&ctx, 1.0f);
}

void sim_step(SimulationState *state, const TickInput *input, float dt, EventQueue *events) {
    if (state->match_over || !b2World_IsValid(state->world)) return;

    StepCtx ctx = { .s = state, .input = input, .dt = dt, .events = events };

    player_system(&ctx);
    enemy_system(&ctx);

    /* Remember each ball's velocity so collisions can be inferred. Dense
     * indices do not change during the physics step. */
    float prev_vx[MAX_BALLS], prev_vy[MAX_BALLS];
    for (uint32_t d = 0; d < state->balls.count; ++d) {
        prev_vx[d] = state->balls.vx[d];
        prev_vy[d] = state->balls.vy[d];
    }

    b2World_Step(state->world, dt, SIM_SUBSTEPS);

    sync_physics_to_state(state);
    collision_events(&ctx, prev_vx, prev_vy);
    scoring_system(&ctx);
    powerups_update(&state->powerups, dt);

    state->tick++;
}

uint64_t sim_config_hash(void) {
    uint64_t h = hash_begin();

    h = hash_u32(h, SIM_LOGIC_VERSION);
    h = hash_u32(h, (uint32_t)SIM_HZ);
    h = hash_u32(h, (uint32_t)SIM_SUBSTEPS);
    h = hash_u32(h, (uint32_t)MAX_BALLS);
    h = hash_u32(h, (uint32_t)WINNING_SCORE);

    h = hash_f32(h, COURT_LEFT);
    h = hash_f32(h, COURT_RIGHT);
    h = hash_f32(h, COURT_TOP);
    h = hash_f32(h, COURT_BOTTOM);
    h = hash_f32(h, PADDLE_WIDTH);
    h = hash_f32(h, PADDLE_HEIGHT);
    h = hash_f32(h, PADDLE_INSET);
    h = hash_f32(h, PLAYER_SPEED);
    h = hash_f32(h, AI_SPEED);
    h = hash_f32(h, AI_DEADZONE_PX);
    h = hash_f32(h, BALL_SIZE);
    h = hash_f32(h, INITIAL_BALL_SPEED);
    h = hash_f32(h, SERVE_ANGLE_MIN_DEG);
    h = hash_f32(h, SERVE_ANGLE_MAX_DEG);

    return h;
}
