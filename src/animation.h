#ifndef ANIMATION_H
#define ANIMATION_H

#include <stdbool.h>
#include "sim_types.h"

typedef struct {
    bool active;
    uint8_t tile_value;
    uint8_t from_index;
    uint8_t to_index;
    float elapsed;
    float duration;
} TileAnimation;

void animation_reset(TileAnimation *animation);
void animation_start(TileAnimation *animation, const MoveResult *move, float duration);
void animation_update(TileAnimation *animation, float frame_time);
float animation_progress(const TileAnimation *animation);

#endif
