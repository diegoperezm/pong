#include "animation.h"

#include <string.h>

void animation_reset(TileAnimation *animation)
{
    if (animation) memset(animation, 0, sizeof(*animation));
}

void animation_start(TileAnimation *animation, const MoveResult *move, float duration)
{
    if (!animation || !move) return;
    animation->active = true;
    animation->tile_value = move->tile_value;
    animation->from_index = move->from_index;
    animation->to_index = move->to_index;
    animation->elapsed = 0.0f;
    animation->duration = duration > 0.0f ? duration : 0.001f;
}

void animation_update(TileAnimation *animation, float frame_time)
{
    if (!animation || !animation->active || frame_time <= 0.0f) return;
    animation->elapsed += frame_time;
    if (animation->elapsed >= animation->duration) {
        animation->elapsed = animation->duration;
        animation->active = false;
    }
}

float animation_progress(const TileAnimation *animation)
{
    float t;
    if (!animation) return 1.0f;
    if (!animation->active) return 1.0f;
    if (animation->duration <= 0.0f) return 1.0f;

    t = animation->elapsed / animation->duration;
    if (t < 0.0f) t = 0.0f;
    if (t > 1.0f) t = 1.0f;

    /* Smoothstep easing: zero velocity at both ends of the visual transition. */
    return t * t * (3.0f - 2.0f * t);
}
