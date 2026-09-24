#include "simulation.h"
#include "entities.h"
#include "log.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

#define PHYSICS_SCALE 100.0f
#define PX_TO_M(x) ((x) / PHYSICS_SCALE)
#define M_TO_PX(x) ((x) * PHYSICS_SCALE)
#define BALL_ANGLE_DEGREES 30.0f
#define PI 3.14159265359

static b2WorldId world;
static b2BodyId  player_body;
static b2BodyId  enemy_body;
static b2BodyId  ball_bodies[MAX_BALLS];
static bool      initialized = false;


// HELPERS
static b2Vec2
pixel_to_meter(float x, float y)
{
  return (b2Vec2){
    PX_TO_M(x),
    PX_TO_M(y)
  };
}

static void
destroy_ball_body(int index)
{
  if (!b2Body_IsValid(ball_bodies[index]))
    return;

  b2DestroyBody(ball_bodies[index]);
  ball_bodies[index] = b2_nullBodyId;
}

static void
destroy_paddle_bodies(void)
{
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

static b2BodyId
create_static_box(
  float x,
  float y,
  float width,
  float height
)
{
  b2BodyDef body_def = b2DefaultBodyDef();
  
  body_def.type = b2_staticBody;
  body_def.position = pixel_to_meter(
      x + width * 0.5f,
      y + height * 0.5f
  );
  
  b2BodyId body        = b2CreateBody(world, &body_def);
  b2ShapeDef shape_def = b2DefaultShapeDef();

  shape_def.material.friction = 0.0f;
  shape_def.material.restitution = 1.0f;
  
  b2Polygon box =
      b2MakeBox(
          PX_TO_M(width * 0.5f),
          PX_TO_M(height * 0.5f)
      );
  
  b2CreatePolygonShape(
      body,
      &shape_def,
      &box
  );
  
  return body;

}

static b2BodyId
create_paddle_body(
  const Paddle *paddle
)
{
  b2BodyDef body_def = b2DefaultBodyDef();
  body_def.type = b2_kinematicBody;
  
  body_def.position =
      pixel_to_meter(
          paddle->x + paddle->width * 0.5f,
          paddle->y + paddle->height * 0.5f
      );
  
  
  b2BodyId body = b2CreateBody(world, &body_def);
  b2ShapeDef shape_def = b2DefaultShapeDef();
  
  shape_def.material.friction = 0.0f;
  shape_def.material.restitution = 1.0f;
  
  b2Polygon box =
      b2MakeBox(
          PX_TO_M(paddle->width * 0.5f),
          PX_TO_M(paddle->height * 0.5f)
      );
  
  b2CreatePolygonShape(
      body,
      &shape_def,
      &box
  );
  return body;
}

static b2BodyId
create_ball_body(
  float x,
  float y,
  float vx,
  float vy
)
{
  b2BodyDef body_def = b2DefaultBodyDef();
  body_def.type = b2_dynamicBody;

  body_def.position =
      pixel_to_meter(
          x + BALL_SIZE * 0.5f,
          y + BALL_SIZE * 0.5f
      );
  
  body_def.isBullet    = true;
  body_def.enableSleep = false;
  
  b2BodyId body = b2CreateBody(world, &body_def);
  b2ShapeDef shape_def = b2DefaultShapeDef();
  
  shape_def.density              = 1.0f;
  shape_def.material.friction    = 0.0f;
  shape_def.material.restitution = 1.0f;
  
  b2Circle circle = {
      .center = {0.0f, 0.0f},
      .radius = PX_TO_M(BALL_SIZE * 0.5f)
  };
  
  b2CreateCircleShape(
      body,
      &shape_def,
      &circle
  );
  
  b2Body_SetLinearVelocity(
      body,
      pixel_to_meter(vx, vy)
  );
  
  return body;

}

// WORLD

static void
create_walls(void)
{

// Top wall.
  create_static_box(
     COURT_LEFT,
     COURT_TOP - 10.0f,
     COURT_RIGHT - COURT_LEFT,
     10.0f
  );

/*
 * Bottom wall.
 */
   create_static_box(
     COURT_LEFT,
     COURT_BOTTOM,
     COURT_RIGHT - COURT_LEFT,
     10.0f
   );
}

// SYNCHRONIZATION
static void
sync_balls_to_physics(SimulationState *state)
{
  BallPool *balls = &state->balls;

  for (int i = 0; i < MAX_BALLS; ++i) {
    EntityHandle handle = {
        .index      = (uint32_t)i,
        .generation = balls->slots[i].generation
    };

    if (!ball_is_valid(balls, handle)) {
        destroy_ball_body(i);
        continue;
    }

    /*
     * Create Box2D body when the
     * game-side ball is newly created.
     */
    if (!b2Body_IsValid(ball_bodies[i])) {
        ball_bodies[i] =
            create_ball_body(
                balls->x[i],
                balls->y[i],
                balls->vx[i],
                balls->vy[i]
            );
        continue;
    }

    /*
     * The game-side state normally comes
     * FROM Box2D, so we don't overwrite
     * the Box2D transform here.
     */
  }
}

static void
sync_physics_to_game(SimulationState *state)
{
  BallPool *balls = &state->balls;

  for (int i = 0; i < MAX_BALLS; ++i) {
    EntityHandle handle = {
        .index = (uint32_t)i,
        .generation = balls->slots[i].generation
    };

    if (!ball_is_valid(balls, handle))
        continue;

    if (!b2Body_IsValid(ball_bodies[i]))
        continue;

    b2Vec2 position = b2Body_GetPosition(ball_bodies[i]);
    b2Vec2 velocity = b2Body_GetLinearVelocity(ball_bodies[i]);

    /*
     * Box2D stores the ball CENTER.
     * The game stores its TOP-LEFT.
     */
    balls->x[i]     = M_TO_PX(position.x) - BALL_SIZE * 0.5f;
    balls->y[i]     = M_TO_PX(position.y) - BALL_SIZE * 0.5f;
    balls->vx[i]    = M_TO_PX(velocity.x);
    balls->vy[i]    = M_TO_PX(velocity.y);
    balls->speed[i] = sqrtf( balls->vx[i] * balls->vx[i] + balls->vy[i] * balls->vy[i]);
}

/*
 * Paddles.
 */
  if (b2Body_IsValid(player_body)) {
    b2Vec2 position = b2Body_GetPosition(player_body);
    state->player.y = M_TO_PX(position.y) - state->player.height * 0.5f;
  }

  if (b2Body_IsValid(enemy_body)) {
    b2Vec2 position = b2Body_GetPosition( enemy_body);
    state->enemy.y = M_TO_PX(position.y) - state->enemy.height * 0.5f;
  }

}

/* ============================================================

* PLAYER / AI
* ============================================================
  */

static void
player_system(
SimulationState *state,
const GameInput *input
)
{
float velocity = 0.0f;

if (input->up)
    velocity -= state->player.speed;

if (input->down)
    velocity += state->player.speed;

b2Body_SetLinearVelocity(
    player_body,
    (b2Vec2){
        0.0f,
        PX_TO_M(velocity)
    }
);

}

static void
enemy_system(
SimulationState *state
)
{
BallPool *balls =
&state->balls;

if (!ball_is_valid(
        balls,
        state->ai_target))
{
    state->ai_target =
        INVALID_HANDLE;

    float best_x =
        -100000.0f;

    for (uint32_t i = 0;
         i < MAX_BALLS;
         ++i)
    {
        if (!balls->slots[i].active)
            continue;

        if (balls->x[i] > best_x) {

            best_x =
                balls->x[i];

            state->ai_target =
                (EntityHandle){
                    .index = i,
                    .generation =
                        balls->slots[i].generation
                };
        }
    }
}

if (!ball_is_valid(
        balls,
        state->ai_target))
{
    b2Body_SetLinearVelocity(
        enemy_body,
        (b2Vec2){0.0f, 0.0f}
    );

    return;
}

uint32_t index =
    state->ai_target.index;

float ball_y =
    balls->y[index] +
    BALL_SIZE * 0.5f;

float paddle_y =
    state->enemy.y +
    state->enemy.height * 0.5f;

float velocity = 0.0f;

if (ball_y < paddle_y)
    velocity = -state->enemy.speed;

else if (ball_y > paddle_y)
    velocity = state->enemy.speed;

b2Body_SetLinearVelocity(
    enemy_body,
    (b2Vec2){
        0.0f,
        PX_TO_M(velocity)
    }
);

}

/* ============================================================

* SCORING
* ============================================================
  */

static void
scoring_system(
SimulationState *state
)
{
BallPool *balls =
&state->balls;

int active_balls = 0;

for (uint32_t i = 0;
     i < MAX_BALLS;
     ++i)
{
    if (!balls->slots[i].active)
        continue;

    EntityHandle handle = {
        .index = i,
        .generation =
            balls->slots[i].generation
    };

    if (balls->x[i] <
        COURT_LEFT - BALL_SIZE)
    {
        state->enemy.score++;
/*
        particles_spawn(
            &state->particles,
            balls->x[i],
            balls->y[i],
            20
        );
*/
particles_spawn(
    &state->particles,
    balls->x[i],
    balls->y[i],
    0.0f,   /* vx */
    0.0f,   /* vy */
    1.0f,   /* lifetime */
    2.0f    /* size */
);
        destroy_ball_body(i);

        ball_destroy(
            balls,
            handle
        );
    }
    else if (balls->x[i] >
             COURT_RIGHT)
    {
        state->player.score++;
/*
        particles_spawn(
            &state->particles,
            balls->x[i],
            balls->y[i],
            20
        );
*/
particles_spawn(
    &state->particles,
    balls->x[i],
    balls->y[i],
    0.0f,   /* vx */
    0.0f,   /* vy */
    1.0f,   /* lifetime */
    2.0f    /* size */
);
        destroy_ball_body(i);

        ball_destroy(
            balls,
            handle
        );
    }
    else {
        active_balls++;
    }
}


  if (active_balls == 0) {
    float angle = 30.0f * (PI / 180.0f);
    float direction =
        (state->player.score > state->enemy.score)
            ? -1.0f
            : 1.0f;


    ball_create(
        balls,
        SCREEN_WIDTH / 2.0f - BALL_SIZE / 2.0f,
        SCREEN_HEIGHT / 2.0f - BALL_SIZE / 2.0f,
        direction
    );
}

}

/* ============================================================

* INITIALIZATION
* ============================================================
  */

void
simulation_init(
SimulationState *state
)
{
b2WorldDef world_def =
b2DefaultWorldDef();

world_def.gravity =
    (b2Vec2){
        0.0f,
        0.0f
    };

world_def.enableContinuous = true;

world =
    b2CreateWorld(
        &world_def
    );

player_body =
    b2_nullBodyId;

enemy_body =
    b2_nullBodyId;

for (int i = 0;
     i < MAX_BALLS;
     ++i)
{
    ball_bodies[i] =
        b2_nullBodyId;
}

create_walls();

initialized = true;

(void)state;

LOG_SIMULATION(
    "Box2D initialized"
);

}

/* ============================================================

* RESET
* ============================================================
  */

void
simulation_reset(
SimulationState *state
)
{
if (!initialized)
return;

destroy_paddle_bodies();

for (int i = 0;
     i < MAX_BALLS;
     ++i)
{
    destroy_ball_body(i);
}

(void)state;

}

/* ============================================================

* SHUTDOWN
* ============================================================
  */

void
simulation_shutdown(void)
{
if (!initialized)
return;

if (b2World_IsValid(world)) {
    b2DestroyWorld(world);
}

world =
    b2_nullWorldId;

initialized = false;

}

/* ============================================================

* MAIN SIMULATION
* ============================================================
  */

void
simulation_update(
  SimulationState *state,
  const GameInput *input,
  float dt
)
{
  if (!initialized)
    return;

/*
 * Create/recreate physics bodies.
 */
if (!b2Body_IsValid(player_body))
    player_body =
        create_paddle_body(
            &state->player
        );

if (!b2Body_IsValid(enemy_body))
    enemy_body =
        create_paddle_body(
            &state->enemy
        );


/*
 * Game-side input/AI -> Box2D.
 */
player_system(state, input);

enemy_system(state);


/*
 * Game-side entities -> Box2D.
 */
sync_balls_to_physics(state);


/*
 * Physics.
 */
b2World_Step(
    world,
    dt,
    4
);


/*
 * Box2D -> game-side state.
 */
sync_physics_to_game(state);


/*
 * Scoring still belongs to the
 * game rules, not the physics engine.
 */
scoring_system(state);


/*
 * Non-physics systems.
 */
particles_update(
    &state->particles,
    dt
);
}
