#ifndef RENDER_H
#define RENDER_H

#include "render_types.h"

typedef struct {
    float x;
    float y;
} RenderVec2;

void render_init(void);
void render_shutdown(void);

void render_frame(
    const RenderSnapshot *previous,
    const RenderSnapshot *current,
    float alpha
);

#endif
