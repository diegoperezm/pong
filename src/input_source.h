#ifndef INPUT_SOURCE_H
#define INPUT_SOURCE_H

#include <stdbool.h>
#include <stdint.h>
#include "game_types.h"

typedef enum { INPUT_SOURCE_LIVE } InputSourceKind;

typedef struct {
    InputSourceKind kind;
    TickInput live_held;
} InputSource;

void input_source_use_live(InputSource *s);
void input_source_set_live_held(InputSource *s, TickInput held);
TickInput input_source_at(InputSource *s, uint32_t tick);

#endif
