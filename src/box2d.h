#ifndef FAKE_BOX2D_H
#define FAKE_BOX2D_H

/* NOT Box2D. A tiny stand-in implementing just the calls this project uses,
 * so the plumbing (queues, replays, seek, checksums, app state machine) can
 * be tested without the real library. It says NOTHING about Box2D's own
 * determinism: for that, build the same test against real Box2D
 * (see tests/Makefile, target `real`). */

#include <stdbool.h>
#include <stdint.h>

typedef struct { float x, y; } b2Vec2;
typedef struct { int32_t index1; uint16_t world0; uint16_t generation; } b2WorldId;
typedef struct { int32_t index1; uint16_t world0; uint16_t generation; } b2BodyId;
typedef struct { int32_t index1; uint16_t world0; uint16_t generation; } b2ShapeId;

typedef enum { b2_staticBody = 0, b2_kinematicBody, b2_dynamicBody } b2BodyType;

typedef struct { b2Vec2 gravity; } b2WorldDef;
typedef struct { b2BodyType type; b2Vec2 position; bool isBullet; bool enableSleep; } b2BodyDef;
typedef struct { float friction; float restitution; } b2SurfaceMaterial;
typedef struct { float density; b2SurfaceMaterial material; } b2ShapeDef;
typedef struct { b2Vec2 center; float radius; } b2Circle;
typedef struct { float hx, hy; } b2Polygon;   /* axis-aligned box only */

static const b2BodyId  b2_nullBodyId  = { 0, 0, 0 };
static const b2WorldId b2_nullWorldId = { 0, 0, 0 };

b2WorldDef b2DefaultWorldDef(void);
b2WorldId  b2CreateWorld(const b2WorldDef *def);
void       b2DestroyWorld(b2WorldId world);
bool       b2World_IsValid(b2WorldId world);
void       b2World_Step(b2WorldId world, float dt, int substeps);

b2BodyDef  b2DefaultBodyDef(void);
b2BodyId   b2CreateBody(b2WorldId world, const b2BodyDef *def);
void       b2DestroyBody(b2BodyId body);
bool       b2Body_IsValid(b2BodyId body);
void       b2Body_SetLinearVelocity(b2BodyId body, b2Vec2 v);
b2Vec2     b2Body_GetLinearVelocity(b2BodyId body);
b2Vec2     b2Body_GetPosition(b2BodyId body);

b2ShapeDef b2DefaultShapeDef(void);
b2Polygon  b2MakeBox(float hx, float hy);
b2ShapeId  b2CreatePolygonShape(b2BodyId body, const b2ShapeDef *def, const b2Polygon *box);
b2ShapeId  b2CreateCircleShape(b2BodyId body, const b2ShapeDef *def, const b2Circle *circle);
bool       b2Shape_IsValid(b2ShapeId shape);

#endif
