#ifndef INPUT_SOURCE_H
#define INPUT_SOURCE_H

#include <stdbool.h>
#include <stdint.h>
#include "sim_types.h"

typedef enum { 
    INPUT_SOURCE_LIVE, 
    INPUT_SOURCE_REPLAY 
} InputSourceKind;

typedef struct {
    InputSourceKind   kind;
    TickInput         live_held;
} InputSource;

void      input_source_use_live(InputSource *s);
void      input_source_set_live_held(InputSource *s, TickInput held);
void      input_source_rewind(InputSource *s);
TickInput input_source_at(InputSource *s, uint32_t tick);   /* ticks in increasing order */

#endif
