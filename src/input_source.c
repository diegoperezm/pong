#include <stddef.h>
#include "input_source.h"

void input_source_use_live(InputSource *s) {
    s->kind      = INPUT_SOURCE_LIVE;
    s->live_held = (TickInput){ false, false };
}



void input_source_set_live_held(InputSource *s, TickInput held) {
    s->live_held = held;
}



TickInput input_source_at(InputSource *s, uint32_t tick) {
    if (s->kind == INPUT_SOURCE_LIVE)
        return s->live_held;
    return (TickInput){ false, false };
}
