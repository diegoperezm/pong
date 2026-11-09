#ifndef INPUT_SOURCE_H
#define INPUT_SOURCE_H

#include <stdbool.h>
#include <stdint.h>
#include "game_types.h"

typedef enum { INPUT_SOURCE_LIVE } InputSourceKind;

typedef struct {
    InputSourceKind kind;
    TickInput pending_input; /* Renamed from live_held per review feedback */
} InputSource;

void input_source_use_live(InputSource *s);
void input_source_set_pending(InputSource *s, TickInput held);
void input_source_consume_one_shot(InputSource *s);
void input_source_clear(InputSource *s);
TickInput input_source_at(InputSource *s, uint32_t tick);

#endif
