#include "box2d/box2d.h"
#include <math.h>
#include <string.h>

#define MAX_WORLDS 4
#define MAX_BODIES 64

typedef struct {
    bool     alive;
    uint16_t gen;
    b2BodyType type;
    b2Vec2   pos, vel;
    int      shape;            /* 0 none, 1 box, 2 circle */
    float    hx, hy, radius;
} FBody;

typedef struct {
    bool     alive;
    uint16_t gen;
    FBody    bodies[MAX_BODIES];
} FWorld;

static FWorld   g_worlds[MAX_WORLDS];
static uint16_t g_next_gen = 1;

static FWorld *world_of(b2WorldId id) {
    if (id.index1 <= 0 || id.index1 > MAX_WORLDS) return NULL;
    FWorld *w = &g_worlds[id.index1 - 1];
    return (w->alive && w->gen == id.generation) ? w : NULL;
}

static FBody *body_of(b2BodyId id) {
    if (id.index1 <= 0 || id.index1 > MAX_BODIES) return NULL;
    if (id.world0 >= MAX_WORLDS) return NULL;
    FWorld *w = &g_worlds[id.world0];
    if (!w->alive) return NULL;
    FBody *b = &w->bodies[id.index1 - 1];
    return (b->alive && b->gen == id.generation) ? b : NULL;
}

b2WorldDef b2DefaultWorldDef(void) { b2WorldDef d; memset(&d, 0, sizeof d); return d; }
b2BodyDef  b2DefaultBodyDef(void)  { b2BodyDef d;  memset(&d, 0, sizeof d); d.type = b2_staticBody; return d; }
b2ShapeDef b2DefaultShapeDef(void) { b2ShapeDef d; memset(&d, 0, sizeof d); d.density = 1.0f; return d; }
b2Polygon  b2MakeBox(float hx, float hy) { return (b2Polygon){ hx, hy }; }

b2WorldId b2CreateWorld(const b2WorldDef *def) {
    (void)def;
    for (int i = 0; i < MAX_WORLDS; ++i) {
        if (g_worlds[i].alive) continue;
        memset(&g_worlds[i], 0, sizeof g_worlds[i]);
        g_worlds[i].alive = true;
        g_worlds[i].gen = g_next_gen++;
        return (b2WorldId){ i + 1, 0, g_worlds[i].gen };
    }
    return b2_nullWorldId;
}

void b2DestroyWorld(b2WorldId id) {
    FWorld *w = world_of(id);
    if (!w) return;
    memset(w, 0, sizeof *w);        /* every body dies with the world */
}

bool b2World_IsValid(b2WorldId id) { return world_of(id) != NULL; }

b2BodyId b2CreateBody(b2WorldId wid, const b2BodyDef *def) {
    FWorld *w = world_of(wid);
    if (!w) return b2_nullBodyId;
    for (int i = 0; i < MAX_BODIES; ++i) {
        if (w->bodies[i].alive) continue;
        FBody *b = &w->bodies[i];
        memset(b, 0, sizeof *b);
        b->alive = true;
        b->gen = g_next_gen++;
        b->type = def->type;
        b->pos = def->position;
        return (b2BodyId){ i + 1, (uint16_t)(wid.index1 - 1), b->gen };
    }
    return b2_nullBodyId;
}

void b2DestroyBody(b2BodyId id) {
    FBody *b = body_of(id);
    if (b) memset(b, 0, sizeof *b);
}

bool b2Body_IsValid(b2BodyId id) { return body_of(id) != NULL; }

void b2Body_SetLinearVelocity(b2BodyId id, b2Vec2 v) { FBody *b = body_of(id); if (b) b->vel = v; }
b2Vec2 b2Body_GetLinearVelocity(b2BodyId id) { FBody *b = body_of(id); return b ? b->vel : (b2Vec2){0, 0}; }
b2Vec2 b2Body_GetPosition(b2BodyId id) { FBody *b = body_of(id); return b ? b->pos : (b2Vec2){0, 0}; }

b2ShapeId b2CreatePolygonShape(b2BodyId id, const b2ShapeDef *def, const b2Polygon *box) {
    (void)def;
    FBody *b = body_of(id);
    if (!b) return (b2ShapeId){0, 0, 0};
    b->shape = 1; b->hx = box->hx; b->hy = box->hy;
    return (b2ShapeId){ id.index1, id.world0, id.generation };
}

b2ShapeId b2CreateCircleShape(b2BodyId id, const b2ShapeDef *def, const b2Circle *c) {
    (void)def;
    FBody *b = body_of(id);
    if (!b) return (b2ShapeId){0, 0, 0};
    b->shape = 2; b->radius = c->radius;
    return (b2ShapeId){ id.index1, id.world0, id.generation };
}

bool b2Shape_IsValid(b2ShapeId s) {
    return body_of((b2BodyId){ s.index1, s.world0, s.generation }) != NULL;
}

static float clampf(float v, float lo, float hi) { return v < lo ? lo : (v > hi ? hi : v); }

static void collide_circle_box(FBody *c, const FBody *b) {
    float cx = clampf(c->pos.x, b->pos.x - b->hx, b->pos.x + b->hx);
    float cy = clampf(c->pos.y, b->pos.y - b->hy, b->pos.y + b->hy);
    float dx = c->pos.x - cx, dy = c->pos.y - cy;
    if (dx * dx + dy * dy >= c->radius * c->radius) return;

    if (fabsf(dx) > fabsf(dy)) {
        float sgn = dx >= 0.0f ? 1.0f : -1.0f;
        if (c->vel.x * sgn < 0.0f) c->vel.x = -c->vel.x;
        c->pos.x = cx + sgn * c->radius;
    } else {
        float sgn = dy >= 0.0f ? 1.0f : -1.0f;
        if (c->vel.y * sgn < 0.0f) c->vel.y = -c->vel.y;
        c->pos.y = cy + sgn * c->radius;
    }
}

void b2World_Step(b2WorldId id, float dt, int substeps) {
    (void)substeps;
    FWorld *w = world_of(id);
    if (!w) return;

    for (int i = 0; i < MAX_BODIES; ++i) {
        FBody *b = &w->bodies[i];
        if (!b->alive || b->type == b2_staticBody) continue;
        b->pos.x += b->vel.x * dt;
        b->pos.y += b->vel.y * dt;
    }
    for (int i = 0; i < MAX_BODIES; ++i) {
        FBody *c = &w->bodies[i];
        if (!c->alive || c->type != b2_dynamicBody || c->shape != 2) continue;
        for (int j = 0; j < MAX_BODIES; ++j) {
            FBody *o = &w->bodies[j];
            if (!o->alive || o->shape != 1) continue;
            collide_circle_box(c, o);
        }
    }
}
