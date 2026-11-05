#ifndef APP_H
#define APP_H

#include <stdint.h>
#include "sim_types.h"
#include "render_types.h"
#include "events.h"
#include "input_source.h"

typedef struct {
    const RenderSnapshot *previous;
    const RenderSnapshot *current;
    float alpha;
} AppView;

typedef struct {
    GameMode mode;
    SimulationState sim;
    EventQueue events;
    InputSource source;
    uint64_t base_seed;
    uint32_t match_counter;
    double accumulator;
    float alpha;
    RenderSnapshot previous;
    RenderSnapshot current;
    char status[96];
} App;

void app_init(App *app, uint64_t base_seed);
void app_shutdown(App *app);
void app_frame(App *app, const GameInput *input, double frame_time);
AppView app_view(const App *app);

#endif
