#ifndef APP_H
#define APP_H

#include <stdint.h>
#include "animation.h"
#include "events.h"
#include "game_types.h"
#include "sim_types.h"

typedef struct {
    GameMode mode;
    SimulationState sim;
    TileAnimation animation;
    EventQueue events;
    uint64_t base_seed;
    uint32_t match_counter;
    char status[96];
} App;

typedef struct {
    GameMode mode;
    const    SimulationState *sim;
    const    TileAnimation *animation;
    const    char *status;
} AppView;

void app_init(App *app, uint64_t base_seed);
void app_shutdown(App *app);
void app_frame(App *app, const GameInput *input, float frame_time);
AppView app_view(const App *app);

#endif
