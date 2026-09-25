#include "simulation.h"
#include "entities.h"
#include "log.h"
#include <math.h>
#include <stdbool.h>

#define PHYSICS_SCALE 100.0f
#define PX_TO_M(x) ((x) / PHYSICS_SCALE)
#define M_TO_PX(x) ((x) * PHYSICS_SCALE)
#define PI 3.14159265359f

static b2WorldId world;
static b2BodyId  player_body;
static b2BodyId  enemy_body;
static b2BodyId  ball_bodies[MAX_BALLS]; // Indexed by sparse slot_idx
static bool      initialized = false;

// HELPERS
static b2Vec2 pixel_to_meter(float x, float y) {
    return (b2Vec2){ PX_TO_M(x), PX_TO_M(y) };
}

static void destroy_ball_body(uint32_t slot_idx) {
    if (slot_idx >= MAX_BALLS || !b2Body_IsValid(ball_bodies[slot_idx]))
        return;

    b2DestroyBody(ball_bodies[slot_idx]);
    ball_bodies[slot_idx] = b2_nullBodyId;
}

static void destroy_paddle_bodies(void) {
    if (b2Body_IsValid(player_body)) {
        b2DestroyBody(player_body);
        player_body = b2_nullBodyId;
    }
    if (b2Body_IsValid(enemy_body)) {
        b2DestroyBody(enemy_body);
        enemy_body = b2_nullBodyId;
    }
}

// BOX2D BODIES
static b2BodyId create_static_box(float x, float y, float width, float height) {
    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type = b2_staticBody;
    body_def.position = pixel_to_meter(x + width * 0.5f, y + height * 0.5f);
    
    b2BodyId body = b2CreateBody(world, &body_def);
    b2ShapeDef shape_def = b2DefaultShapeDef();
    shape_def.material.friction = 0.0f;
    shape_def.material.restitution = 1.0f;
    
    b2Polygon box = b2MakeBox(PX_TO_M(width * 0.5f), PX_TO_M(height * 0.5f));
    b2CreatePolygonShape(body, &shape_def, &box);
    return body;
}

static b2BodyId create_paddle_body(const Paddle *paddle) {
    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type = b2_kinematicBody;
    body_def.position = pixel_to_meter(paddle->x + paddle->width * 0.5f, paddle->y + paddle->height * 0.5f);
    
    b2BodyId body = b2CreateBody(world, &body_def);
    b2ShapeDef shape_def = b2DefaultShapeDef();
    shape_def.material.friction = 0.0f;
    shape_def.material.restitution = 1.0f;
    
    b2Polygon box = b2MakeBox(PX_TO_M(paddle->width * 0.5f), PX_TO_M(paddle->height * 0.5f));
    b2CreatePolygonShape(body, &shape_def, &box);
    return body;
}

static b2BodyId create_ball_body(float x, float y, float vx, float vy) {
    b2BodyDef body_def = b2DefaultBodyDef();
    body_def.type = b2_dynamicBody;
    body_def.position = pixel_to_meter(x + BALL_SIZE * 0.5f, y + BALL_SIZE * 0.5f);
    body_def.isBullet = true;
    body_def.enableSleep = false;
    
    b2BodyId body = b2CreateBody(world, &body_def);
    b2ShapeDef shape_def = b2DefaultShapeDef();
    shape_def.density = 1.0f;
    shape_def.material.friction = 0.0f;
    shape_def.material.restitution = 1.0f;
    
    b2Circle circle = { .center = {0.0f, 0.0f}, .radius = PX_TO_M(BALL_SIZE * 0.5f) };
    b2CreateCircleShape(body, &shape_def, &circle);
    b2Body_SetLinearVelocity(body, pixel_to_meter(vx, vy));
    return body;
}

static void create_walls(void) {
    create_static_box(COURT_LEFT, COURT_TOP - 10.0f, COURT_RIGHT - COURT_LEFT, 10.0f);
    create_static_box(COURT_LEFT, COURT_BOTTOM, COURT_RIGHT - COURT_LEFT, 10.0f);
}

// SYNCHRONIZATION
static void sync_balls_to_physics(SimulationState *state) {
    BallPool *balls = &state->balls;

    // Direct iteration over contiguous dense array
    for (uint32_t d = 0; d < balls->count; ++d) {
        uint32_t slot_idx = balls->dense_to_sparse[d];

        if (!b2Body_IsValid(ball_bodies[slot_idx])) {
            ball_bodies[slot_idx] = create_ball_body(
                balls->x[d],
                balls->y[d],
                balls->vx[d],
                balls->vy[d]
            );
        }
    }
}

static void sync_physics_to_game(SimulationState *state) {
    BallPool *balls = &state->balls;

    // Direct iteration over contiguous dense array
    for (uint32_t d = 0; d < balls->count; ++d) {
        uint32_t slot_idx = balls->dense_to_sparse[d];
        if (!b2Body_IsValid(ball_bodies[slot_idx]))
            continue;

        b2Vec2 position = b2Body_GetPosition(ball_bodies[slot_idx]);
        b2Vec2 velocity = b2Body_GetLinearVelocity(ball_bodies[slot_idx]);

        balls->x[d]     = M_TO_PX(position.x) - BALL_SIZE * 0.5f;
        balls->y[d]     = M_TO_PX(position.y) - BALL_SIZE * 0.5f;
        balls->vx[d]    = M_TO_PX(velocity.x);
        balls->vy[d]    = M_TO_PX(velocity.y);
        balls->speed[d] = sqrtf(balls->vx[d] * balls->vx[d] + balls->vy[d] * balls->vy[d]);
    }

    if (b2Body_IsValid(player_body)) {
        b2Vec2 position = b2Body_GetPosition(player_body);
        state->player.y = M_TO_PX(position.y) - state->player.height * 0.5f;
    }

    if (b2Body_IsValid(enemy_body)) {
        b2Vec2 position = b2Body_GetPosition(enemy_body);
        state->enemy.y = M_TO_PX(position.y) - state->enemy.height * 0.5f;
    }
}

static void player_system(SimulationState *state, const GameInput *input) {
    float velocity = 0.0f;
    if (input->up)   velocity -= state->player.speed;
    if (input->down) velocity += state->player.speed;

    b2Body_SetLinearVelocity(player_body, (b2Vec2){0.0f, PX_TO_M(velocity)});
}

static void enemy_system(SimulationState *state) {
    BallPool *balls = &state->balls;

    if (!ball_is_valid(balls, state->ai_target)) {
        state->ai_target = INVALID_HANDLE;
        float best_x = -100000.0f;

        for (uint32_t d = 0; d < balls->count; ++d) {
            if (balls->x[d] > best_x) {
                best_x = balls->x[d];
                uint32_t slot_idx = balls->dense_to_sparse[d];
                state->ai_target = (EntityHandle){
                    .index = slot_idx,
                    .generation = balls->slots[slot_idx].generation
                };
            }
        }
    }

    if (!ball_is_valid(balls, state->ai_target)) {
        b2Body_SetLinearVelocity(enemy_body, (b2Vec2){0.0f, 0.0f});
        return;
    }

    uint32_t target_dense = balls->slots[state->ai_target.index].dense_idx;
    float ball_y = balls->y[target_dense] + BALL_SIZE * 0.5f;
    float paddle_y = state->enemy.y + state->enemy.height * 0.5f;
    float velocity = 0.0f;

    if (ball_y < paddle_y)      velocity = -state->enemy.speed;
    else if (ball_y > paddle_y) velocity = state->enemy.speed;

    b2Body_SetLinearVelocity(enemy_body, (b2Vec2){0.0f, PX_TO_M(velocity)});
}

static void scoring_system(SimulationState *state) {
    BallPool *balls = &state->balls;

    // Iterate backwards so swap-and-pop does not skip unprocessed elements
    for (int d = (int)balls->count - 1; d >= 0; --d) {
        uint32_t slot_idx = balls->dense_to_sparse[d];
        EntityHandle handle = {
            .index = slot_idx,
            .generation = balls->slots[slot_idx].generation
        };

        if (balls->x[d] < COURT_LEFT - BALL_SIZE) {
            state->enemy.score++;
            particles_spawn(&state->particles, balls->x[d], balls->y[d], 0.0f, 0.0f, 1.0f, 2.0f);
            destroy_ball_body(slot_idx);
            ball_destroy(balls, handle);
        }
        else if (balls->x[d] > COURT_RIGHT) {
            state->player.score++;
            particles_spawn(&state->particles, balls->x[d], balls->y[d], 0.0f, 0.0f, 1.0f, 2.0f);
            destroy_ball_body(slot_idx);
            ball_destroy(balls, handle);
        }
    }

    if (balls->count == 0) {
        float direction = (state->player.score > state->enemy.score) ? -1.0f : 1.0f;
        ball_create(
            balls,
            SCREEN_WIDTH / 2.0f - BALL_SIZE / 2.0f,
            SCREEN_HEIGHT / 2.0f - BALL_SIZE / 2.0f,
            direction
        );
    }
}

void simulation_init(SimulationState *state) {
    b2WorldDef world_def = b2DefaultWorldDef();
    world_def.gravity = (b2Vec2){0.0f, 0.0f};
    world_def.enableContinuous = true;

    world = b2CreateWorld(&world_def);
    player_body = b2_nullBodyId;
    enemy_body  = b2_nullBodyId;

    for (int i = 0; i < MAX_BALLS; ++i) {
        ball_bodies[i] = b2_nullBodyId;
    }

    create_walls();
    initialized = true;
    (void)state;

    LOG_SIMULATION("Box2D initialized");
}

void simulation_reset(SimulationState *state) {
    if (!initialized) return;

    destroy_paddle_bodies();
    for (uint32_t i = 0; i < MAX_BALLS; ++i) {
        destroy_ball_body(i);
    }
    (void)state;
}

void simulation_shutdown(void) {
    if (!initialized) return;

    if (b2World_IsValid(world)) {
        b2DestroyWorld(world);
    }

    world = b2_nullWorldId;
    initialized = false;
}

void simulation_update(SimulationState *state, const GameInput *input, float dt) {
    if (!initialized) return;

    if (!b2Body_IsValid(player_body))
        player_body = create_paddle_body(&state->player);

    if (!b2Body_IsValid(enemy_body))
        enemy_body = create_paddle_body(&state->enemy);

    player_system(state, input);
    enemy_system(state);
    sync_balls_to_physics(state);

    b2World_Step(world, dt, 4);

    sync_physics_to_game(state);
    scoring_system(state);
    particles_update(&state->particles, dt);
}

