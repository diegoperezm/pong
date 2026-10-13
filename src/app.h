#ifndef APP_H
#define APP_H

/* The application layer: everything that is NOT the simulation.
 *
 * Owns the title / playing / paused / game-over state machine, both queues,
 * the particles (fx), recording, replay playback and seeking, and the
 * render-snapshot interpolation state. It is the only code that talks to
 * the simulation, and it does so through three things: a seed, one
 * TickInput per tick, and an event queue.
 *
 * It does not call raylib, so it can be driven headlessly (see tests/). */

#include <stdint.h>
#include "sim_types.h"
#include "render_types.h"
#include "events.h"
#include "fx.h"
#include "input_source.h"
#include "debug.h"


typedef struct {
    const RenderSnapshot *previous;
    const RenderSnapshot *current;
    float                 alpha;
} AppView;

typedef struct {
    GameMode        mode;
    SimulationState sim;
    EventQueue      events;
    Fx              fx;
    InputSource     source;
    uint64_t        base_seed;
    uint32_t        match_counter;
    double          accumulator;
    float           alpha;
    RenderSnapshot  previous;
    RenderSnapshot  current;
    char            status[96];
} App;

void app_init(App *app, uint64_t base_seed);
void app_shutdown(App *app);

/* One call per rendered frame. `frame_time` is the CAPPED frame duration. */
void app_frame(App *app, const GameInput *input, double frame_time);

AppView      app_view(const App *app);
DebugHistory app_debug_history(const App *app);

#endif
