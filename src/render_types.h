#ifndef RENDER_TYPES_H
#define RENDER_TYPES_H

#include <stdint.h>
#include <stdbool.h>
#include "config.h"
#include "sim_types.h"

typedef struct {
    float x, y;
    float vx, vy;
    uint32_t serial;
} RenderEntity;

typedef struct {
    bool  active;
    float x, y;
    float lifetime;
    float max_lifetime;
    float size;
} RenderParticle;

typedef struct {
    GameMode mode;
    int      winner;
    uint32_t tick;
    float    game_time;
    
    int          entity_count;
    RenderEntity entities[MAX_ENTITIES];
    RenderParticle particles[MAX_PARTICLES];
} RenderSnapshot;

#endif
