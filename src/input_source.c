#include <stddef.h>
#include "input_source.h"

void input_source_use_live(InputSource *s) {
    s->kind      = INPUT_SOURCE_LIVE;
    s->live_held = (TickInput){ false, false };
    s->replay    = NULL;
    replay_cursor_reset(&s->cursor);
}

void input_source_use_replay(InputSource *s, const ReplayData *data) {
    s->kind      = INPUT_SOURCE_REPLAY;
    s->live_held = (TickInput){ false, false };
    s->replay    = data;
    replay_cursor_reset(&s->cursor);
}

void input_source_set_live_held(InputSource *s, TickInput held) {
    s->live_held = held;
}

void input_source_rewind(InputSource *s) {
    replay_cursor_reset(&s->cursor);
}

TickInput input_source_at(InputSource *s, uint32_t tick) {
    if (s->kind == INPUT_SOURCE_REPLAY && s->replay)
        return replay_input_at(s->replay, &s->cursor, tick);
    if (s->kind == INPUT_SOURCE_LIVE)
        return s->live_held;
    return (TickInput){ false, false };
}
