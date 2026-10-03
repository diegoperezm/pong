#include "fx.h"
#include <math.h>
#include <string.h>

#define FX_PI 3.14159265358979f

static void particles_clear(ParticlePool *particles) {
    particles->free_head = 0;

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        particles->active[i] = 0;
        particles->next_free[i] = (i + 1 < MAX_PARTICLES) ? (i + 1) : -1;
    }
}

static void particles_spawn(ParticlePool *particles, float x, float y,
                            float vx, float vy, float lifetime, float size) {
    if (particles->free_head < 0) return;   /* pool full */

    int slot = particles->free_head;
    particles->free_head = particles->next_free[slot];

    particles->active[slot]       = 1;
    particles->x[slot]            = x;
    particles->y[slot]            = y;
    particles->vx[slot]           = vx;
    particles->vy[slot]           = vy;
    particles->max_lifetime[slot] = lifetime;
    particles->lifetime[slot]     = lifetime;
    particles->size[slot]         = size;
}

static void particles_update(ParticlePool *particles, float dt) {
    /* Frame-rate independent damping (normalised to 60 FPS). */
    float damping = powf(0.98f, dt * 60.0f);

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        if (!particles->active[i]) continue;

        particles->lifetime[i] -= dt;
        if (particles->lifetime[i] <= 0.0f) {
            particles->active[i] = 0;
            particles->next_free[i] = particles->free_head;
            particles->free_head    = i;
            continue;
        }

        particles->x[i]  += particles->vx[i] * dt;
        particles->y[i]  += particles->vy[i] * dt;
        particles->vx[i] *= damping;
        particles->vy[i] *= damping;
    }
}

static float clampf(float v, float lo, float hi) {
    return v < lo ? lo : (v > hi ? hi : v);
}

/* One RNG draw per statement, so the draw order is fixed. */
static void spawn_score_burst(Fx *fx, float x, float y) {
    /* The ball is already outside the court when it scores: pull the burst
     * back to the court edge so it is actually visible. */
    x = clampf(x, COURT_LEFT, COURT_RIGHT);
    y = clampf(y, COURT_TOP, COURT_BOTTOM);

    for (int p = 0; p < PARTICLE_COUNT_ON_SCORE; ++p) {
        float angle    = (float)rng_range(&fx->rng, 0, 359) * (FX_PI / 180.0f);
        float speed    = (float)rng_range(&fx->rng, PARTICLE_MIN_SPEED, PARTICLE_MAX_SPEED);
        float lifetime = (float)rng_range(&fx->rng, PARTICLE_MIN_LIFETIME, PARTICLE_MAX_LIFETIME) / 1000.0f;
        float size     = (float)rng_range(&fx->rng, PARTICLE_MIN_SIZE, PARTICLE_MAX_SIZE);

        particles_spawn(&fx->particles, x, y,
                        cosf(angle) * speed, sinf(angle) * speed, lifetime, size);
    }
}

void fx_reset(Fx *fx, uint64_t seed, const EventQueue *queue) {
    memset(fx, 0, sizeof *fx);
    particles_clear(&fx->particles);
    rng_seed(&fx->rng, seed, 2);
    event_cursor_sync(queue, &fx->cursor);
}

void fx_consume_events(Fx *fx, const EventQueue *queue) {
    SimEvent e;
    while (event_cursor_read(queue, &fx->cursor, &e)) {
        switch (e.type) {
            case EVT_POINT_SCORED:
                spawn_score_burst(fx, e.x, e.y);
                break;
            default:
                break;   /* other events: future consumers (sound, shake...) */
        }
    }
}

void fx_update(Fx *fx, float dt) {
    particles_update(&fx->particles, dt);
}
