#include "simulation.h"
#include "entities.h"
#include "log.h"
#include "raylib.h"
#include <math.h>


static void destroy_paddle_bodies(SimulationState* state) {
    if (b2Body_IsValid(state->player_body)) {
        b2DestroyBody(state->player_body);
    }
    state->player_body = b2_nullBodyId;

    if (b2Body_IsValid(state->enemy_body)) {
        b2DestroyBody(state->enemy_body);
    }
    state->enemy_body = b2_nullBodyId;
}


void simulation_reset_paddles(SimulationState *state) {
    if (b2Body_IsValid(state->player_body)) {
        b2Vec2 pos = pixel_to_meter(state->player.x + state->player.width * 0.5f,
                                   state->player.y + state->player.height * 0.5f);
        b2Body_SetTransform(state->player_body, pos, (b2Rot){1.0f, 0.0f});
        b2Body_SetLinearVelocity(state->player_body, (b2Vec2){0.0f, 0.0f});
    }

    if (b2Body_IsValid(state->enemy_body)) {
        b2Vec2 pos = pixel_to_meter(state->enemy.x + state->enemy.width * 0.5f,
                                   state->enemy.y + state->enemy.height * 0.5f);
        b2Body_SetTransform(state->enemy_body, pos, (b2Rot){1.0f, 0.0f});
        b2Body_SetLinearVelocity(state->enemy_body, (b2Vec2){0.0f, 0.0f});
    }
}


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
    create_static_box(world, COURT_LEFT, COURT_TOP - 10.0f, COURT_RIGHT - COURT_LEFT, 10.0f);
    create_static_box(world, COURT_LEFT, COURT_BOTTOM, COURT_RIGHT - COURT_LEFT, 10.0f);
}

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


static void player_system(SimulationState *state, const GameInput *input, float dt) {
    if (!b2Body_IsValid(state->player_body)) return;

    float velocity = 0.0f;
    if (input->up)   velocity -= state->player.speed;
    if (input->down) velocity += state->player.speed;

    // Predictive velocity clamping: cap velocity so paddle lands exactly on the bound after dt
    if (dt > 0.0f) {
        float next_y = state->player.y + velocity * dt;
        if (next_y < COURT_TOP) {
            velocity = (COURT_TOP - state->player.y) / dt;
        } else if (next_y > COURT_BOTTOM - state->player.height) {
            velocity = (COURT_BOTTOM - state->player.height - state->player.y) / dt;
        }
    }

    b2Body_SetLinearVelocity(state->player_body, (b2Vec2){0.0f, PX_TO_M(velocity)});
}

static void enemy_system(SimulationState *state, float dt) {
    if (!b2Body_IsValid(state->enemy_body)) return;

    BallPool *balls = &state->balls;

    float best_x = -100000.0f;
    EntityHandle best_target = INVALID_HANDLE;

    for (uint32_t d = 0; d < balls->count; ++d) {
        if (balls->x[d] > best_x) {
            best_x = balls->x[d];
            uint32_t slot_idx = balls->dense_to_sparse[d];
            best_target = (EntityHandle){
                .index = slot_idx,
                .generation = balls->slots[slot_idx].generation
            };
        }
    }
    state->ai_target = best_target;

    if (!ball_is_valid(balls, state->ai_target)) {
        b2Body_SetLinearVelocity(state->enemy_body, (b2Vec2){0.0f, 0.0f});
    } else {
        uint32_t target_dense = balls->slots[state->ai_target.index].dense_idx;
        float ball_y   = balls->y[target_dense] + BALL_SIZE * 0.5f;
        float paddle_y = state->enemy.y + state->enemy.height * 0.5f;
        float diff     = ball_y - paddle_y;
        float velocity = 0.0f;

        if (fabsf(diff) > 6.0f) {
            velocity = (diff < 0.0f) ? -state->enemy.speed : state->enemy.speed;
        }

        // Predictive velocity clamping for AI
        if (dt > 0.0f) {
            float next_y = state->enemy.y + velocity * dt;
            if (next_y < COURT_TOP) {
                velocity = (COURT_TOP - state->enemy.y) / dt;
            } else if (next_y > COURT_BOTTOM - state->enemy.height) {
                velocity = (COURT_BOTTOM - state->enemy.height - state->enemy.y) / dt;
            }
        }

        b2Body_SetLinearVelocity(state->enemy_body, (b2Vec2){0.0f, PX_TO_M(velocity)});
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

    if (state->player.score >= WINNING_SCORE) {
        state->mode = GAME_OVER;
        state->winner = 1;
        return;
    } else if (state->enemy.score >= WINNING_SCORE) {
        state->mode = GAME_OVER;
        state->winner = 2;
        return;
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

    state->world = b2CreateWorld(&world_def);
    state->player_body = b2_nullBodyId;
    state->enemy_body  = b2_nullBodyId;

    create_walls(state->world);

    LOG_SIMULATION("Box2D initialized");
}

void simulation_reset(SimulationState *state) {
    if (!b2World_IsValid(state->world)) return;
      simulation_reset_paddles(state);  
//    destroy_paddle_bodies(state);
    balls_clear(&state->balls);
}

void simulation_shutdown(SimulationState *state) {
    if (!b2World_IsValid(state->world)) return;

    destroy_paddle_bodies(state);
    balls_clear(&state->balls);

    if (b2World_IsValid(state->world)) {
        b2DestroyWorld(state->world);
    }

    state->world = b2_nullWorldId;
}

void simulation_update(SimulationState *state, const GameInput *input, float dt) {
    if (!b2World_IsValid(state->world)) return;

    if (!b2Body_IsValid(state->player_body))
        state->player_body = create_paddle_body(state->world, &state->player);

    if (!b2Body_IsValid(state->enemy_body))
        state->enemy_body = create_paddle_body(state->world, &state->enemy);

    player_system(state, input, dt);
    enemy_system(state, dt);

    b2World_Step(state->world, dt, 4);

    sync_physics_to_state(state);
    scoring_system(state);
    
    particles_update(&state->particles, dt);
    powerups_update(&state->powerups, dt);
}







