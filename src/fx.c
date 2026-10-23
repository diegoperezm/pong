#include "fx.h"
#include <math.h>
#include <string.h>

static void particles_clear(ParticlePool *particles) {
    particles->free_head = 0;

    for (int i = 0; i < MAX_PARTICLES; ++i) {
        particles->active[i] = 0;
        particles->next_free[i] = (i + 1 < MAX_PARTICLES) ? (i + 1) : -1;
    }
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
            /* TODO: Handle custom game events for visual effects here 
             * (e.g., spawn particles on damage, explosions, scoring, etc.) */
            default:
                break;
        }
    }
}

void fx_update(Fx *fx, float dt) {
    particles_update(&fx->particles, dt);
}
