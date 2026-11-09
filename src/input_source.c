#include "input_source.h"

void input_source_use_live(InputSource *s) {
    if (!s) return;
    s->kind = INPUT_SOURCE_LIVE;
    s->pending_input = (TickInput){ DIR_NONE };
}

void input_source_set_pending(InputSource *s, TickInput held) {
    if (!s) return;
    s->pending_input = held;
}

void input_source_consume_one_shot(InputSource *s) {
    if (!s) return;
    s->pending_input.move_dir = DIR_NONE;
}

void input_source_clear(InputSource *s) {
    if (!s) return;
    s->pending_input = (TickInput){ DIR_NONE };
}

TickInput input_source_at(InputSource *s, uint32_t tick) {
    (void)tick;
    if (s && s->kind == INPUT_SOURCE_LIVE)
        return s->pending_input;
    return (TickInput){ DIR_NONE };
}
