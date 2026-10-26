#ifndef INPUT_SOURCE_H
#define INPUT_SOURCE_H

/* One interface in front of every producer of per-tick simulation input.
 * The simulation cannot tell them apart, which is what makes replays and
 * headless tests possible.
 *
 *   LIVE   - the keyboard. The app sets the held state once per frame and
 *            every tick of that frame sees it. 
 */

#include <stdbool.h>
#include <stdint.h>
#include "game_types.h"

typedef enum { INPUT_SOURCE_LIVE } InputSourceKind;

typedef struct {
    InputSourceKind   kind;
    TickInput         live_held;
} InputSource;

void      input_source_use_live(InputSource *s);
void      input_source_set_live_held(InputSource *s, TickInput held);
void      input_source_rewind(InputSource *s);
TickInput input_source_at(InputSource *s, uint32_t tick);   /* ticks in increasing order */

#endif
