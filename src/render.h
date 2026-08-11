#ifndef RENDER_H
#define RENDER_H

#include "types.h"


void render_game(
    const SimulationState *state,
    const RenderSnapshot *previous,
    const RenderSnapshot *current,
    float alpha
);

#endif
