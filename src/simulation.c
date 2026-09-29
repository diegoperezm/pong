#include "simulation.h"
#include "entities.h"
#include "log.h"
#include "raylib.h"
#include <math.h>

static b2BodyId  player_body;
static b2BodyId  enemy_body;
static bool      initialized = false;

static b2Vec2 pixel_to_meter(float x, float y) {
    return (b2Vec2){ PX_TO_M(x), PX_TO_M(y) };
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

static b2BodyId create_static_box(b2WorldId world, float x, float y, float width, float height) {
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

static b2BodyId create_paddle_body(b2WorldId world, const Paddle *paddle) {
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

static void create_walls(b2WorldId world) {
    create_static_box(world, COURT_LEFT, COURT_TOP - 10.0f, COURT_RIGHT - COURT_LEFT, 10.0f);
    create_static_box(world, COURT_LEFT, COURT_BOTTOM, COURT_RIGHT - COURT_LEFT, 10.0f);
}

// SIMULATION SYNCHRONIZATION (Pulls Box2D data back into pure SimulationState)
static void sync_physics_to_state(SimulationState *state) {
    if (b2Body_IsValid(player_body)) {
        b2Vec2 pos = b2Body_GetPosition(player_body);
        state->player.y = M_TO_PX(pos.y) - state->player.height * 0.5f;
    }
    if (b2Body_IsValid(enemy_body)) {
        b2Vec2 pos = b2Body_GetPosition(enemy_body);
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

// SYSTEMS
static void player_system(SimulationState *state, const GameInput *input) {
    float velocity = 0.0f;
    if (input->up)   velocity -= state->player.speed;
    if (input->down) velocity += state->player.speed;

    b2Body_SetLinearVelocity(player_body, (b2Vec2){0.0f, PX_TO_M(velocity)});
}

static void enemy_system(SimulationState *state) {
    BallPool *balls = &state->balls;

    // AI targeting logic reading clean state arrays rather than b2Body_GetPosition
    if (!ball_is_valid(balls, state->ai_target)) {
        state->ai_target = INVALID_HANDLE;
        float best_x = -100000.0f;

        for (uint32_t d = 0; d < balls->count; ++d) {
            float px = balls->x[d];
            if (px > best_x) {
                best_x = px;
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
    } else {
        uint32_t target_dense = balls->slots[state->ai_target.index].dense_idx;
        float ball_y   = balls->y[target_dense] + BALL_SIZE * 0.5f;
        float paddle_y = state->enemy.y + state->enemy.height * 0.5f;
        float velocity = 0.0f;

        if (ball_y < paddle_y)      velocity = -state->enemy.speed;
        else if (ball_y > paddle_y) velocity = state->enemy.speed;

        b2Body_SetLinearVelocity(enemy_body, (b2Vec2){0.0f, PX_TO_M(velocity)});
    }
}

static void scoring_system(SimulationState *state) {
    BallPool *balls = &state->balls;

    for (int d = (int)balls->count - 1; d >= 0; --d) {
        float px = balls->x[d];
        float py = balls->y[d];

        uint32_t slot_idx = balls->dense_to_sparse[d];
        EntityHandle handle = {
            .index = slot_idx,
            .generation = balls->slots[slot_idx].generation
        };

        if (px < COURT_LEFT - BALL_SIZE) {
            state->enemy.score++;
            for (int p = 0; p < PARTICLE_COUNT_ON_SCORE; ++p) {
                float angle = (float)GetRandomValue(0, 359) * (PI / 180.0f);
                float speed = (float)GetRandomValue(PARTICLE_MIN_SPEED, PARTICLE_MAX_SPEED);
                float lifetime = (float)GetRandomValue(PARTICLE_MIN_LIFETIME, PARTICLE_MAX_LIFETIME) / 1000.0f;
                float size = (float)GetRandomValue(PARTICLE_MIN_SIZE, PARTICLE_MAX_SIZE);
                particles_spawn(&state->particles, px, py, cosf(angle) * speed, sinf(angle) * speed, lifetime, size);
            }
            ball_destroy(balls, handle);
        }
        else if (px > COURT_RIGHT) {
            state->player.score++;
            for (int p = 0; p < PARTICLE_COUNT_ON_SCORE; ++p) {
                float angle = (float)GetRandomValue(0, 359) * (PI / 180.0f);
                float speed = (float)GetRandomValue(PARTICLE_MIN_SPEED, PARTICLE_MAX_SPEED);
                float lifetime = (float)GetRandomValue(PARTICLE_MIN_LIFETIME, PARTICLE_MAX_LIFETIME) / 1000.0f;
                float size = (float)GetRandomValue(PARTICLE_MIN_SIZE, PARTICLE_MAX_SIZE);
                particles_spawn(&state->particles, px, py, cosf(angle) * speed, sinf(angle) * speed, lifetime, size);
            }
            ball_destroy(balls, handle);
        }
    }

    if (balls->count == 0) {
        float direction = (state->player.score > state->enemy.score) ? -1.0f : 1.0f;
        ball_create(
            balls,
            state->world,
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

    state->world = b2CreateWorld(&world_def);
    player_body = b2_nullBodyId;
    enemy_body  = b2_nullBodyId;

    create_walls(state->world);
    initialized = true;

    LOG_SIMULATION("Box2D initialized");
}

void simulation_reset(SimulationState *state) {
    if (!initialized) return;
    destroy_paddle_bodies();
    balls_clear(&state->balls);
}

void simulation_shutdown(SimulationState *state) {
    if (!initialized) return;

    if (b2World_IsValid(state->world)) {
        b2DestroyWorld(state->world);
    }

    state->world = b2_nullWorldId;
    initialized = false;
}

void simulation_update(SimulationState *state, const GameInput *input, float dt) {
    if (!initialized) return;

    if (!b2Body_IsValid(player_body))
        player_body = create_paddle_body(state->world, &state->player);

    if (!b2Body_IsValid(enemy_body))
        enemy_body = create_paddle_body(state->world, &state->enemy);

    player_system(state, input);
    enemy_system(state);

    b2World_Step(state->world, dt, 4);

    // Sync physics output into state for downstream readers
    sync_physics_to_state(state);

    scoring_system(state);
    particles_update(&state->particles, dt);
}


