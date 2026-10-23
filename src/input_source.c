#include <stddef.h>
#include "input_source.h"

void input_source_use_live(InputSource *s) {
    s->kind      = INPUT_SOURCE_LIVE;
    s->live_held = (TickInput){0};
}

void input_source_set_live_held(InputSource *s, TickInput held) {
    s->live_held = held;
}

void input_source_rewind(InputSource *s) {
    (void)s;
    /* TODO: Reset replay stream pointers if using recorded replays */
}

TickInput input_source_at(InputSource *s, uint32_t tick) {
    (void)tick;
    if (s->kind == INPUT_SOURCE_LIVE)
        return s->live_held;
    
    /* TODO: Return recorded tick input from replay stream when kind == INPUT_SOURCE_REPLAY */
    return (TickInput){0};
}
