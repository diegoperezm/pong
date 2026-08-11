#ifndef COLLISION_H
#define COLLISION_H

#include "types.h"

int rectangles_overlap(
    float ax,
    float ay,
    float aw,
    float ah,

    float bx,
    float by,
    float bw,
    float bh
);


void collision_update(
    SimulationState *state
);

#endif
