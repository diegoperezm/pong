#ifndef RENDER_H
#define RENDER_H

#include "types.h"

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

#endif

