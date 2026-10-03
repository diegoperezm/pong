#ifndef RENDER_H
#define RENDER_H

#include "render_types.h"

typedef struct {
    float x;
    float y;
} RenderVec2;

void
render_init(
void
);

void
render_shutdown(
void
);

void
render_frame(
const RenderSnapshot *previous,
const RenderSnapshot *current,
float alpha
);

/* Interpolated positions. render_frame() and the debug overlay both use
 * these, so what is outlined is always exactly what is drawn. */
RenderVec2
render_player_position(
const RenderSnapshot *previous,
const RenderSnapshot *current,
float alpha
);

RenderVec2
render_enemy_position(
const RenderSnapshot *previous,
const RenderSnapshot *current,
float alpha
);

/* Returns false if the ball slot is not active in `current`.
 * Interpolates only when the slot held the same generation previously. */
bool
render_ball_position(
const RenderSnapshot *previous,
const RenderSnapshot *current,
int slot,
float alpha,
RenderVec2 *out
);

#endif
