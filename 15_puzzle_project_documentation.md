# 15-Puzzle (Discrete Logic & Visual Interpolation)

A clean, modular C99 implementation of the classic 15-Puzzle using **Raylib** for visual rendering. The project architecture strictly decouples pure, deterministic simulation logic from visual state interpolation, input handling, and event streaming.

---

## 1. Project Overview & Architecture

The application is structured into four core layers:

1. **Simulation Layer (`simulation.c/h`)**: Pure discrete board logic. It handles move validation, tile swapping, win checks, and solvability-guaranteed board shuffles using a deterministic PRNG.
2. **Animation Layer (`animation.c/h`)**: Manages non-blocking visual transitions. Uses smoothstep easing ($t^2 \times (3 - 2t)$) to smoothly interpolate moving tiles without altering the discrete simulation state during movement.
3. **Event Subsystem (`events.c/h`)**: A static, lock-free ring buffer system tracking match starts, tile slides, and puzzle completions for debug and audit tracking.
4. **Application & Render Layer (`app.c/h`, `render.c/h`, `debug.c/h`)**: Enforces state transitions (`GAME_TITLE`, `GAME_PLAYING`, `GAME_PAUSED`, `GAME_OVER`), serializes input while animations are in progress, and handles UI rendering via Raylib.

---

## 2. Compilation and Setup

### Prerequisites
- C99-compliant C compiler (`gcc`, `clang`, or MSVC)
- CMake (v3.12+) or a direct build script
- Raylib library headers and binaries installed on your system path

### Build Instructions (GCC Command Line Example)

```bash
# Compile and link with Raylib
gcc -std=c99 -Wall -Wextra \
    app.c animation.c debug.c events.c input.c log.c main.c render.c rng.c simulation.c \
    -lraylib -lm -lGL -ldl -lrt -lX11 \
    -o puzzle15

# Execute the application
./puzzle15
```

---

## 3. Configuration

Key simulation and timing parameters are configured in `config.h`:

```c
#define BOARD_SIZE 4
#define TOTAL_TILES (BOARD_SIZE * BOARD_SIZE)
#define SHUFFLE_MOVE_COUNT 120
#define SCREEN_WIDTH 800
#define SCREEN_HEIGHT 600
#define TARGET_FPS 60
#define MAX_FRAME_TIME 0.1f

/* Visual animation length (adjust to speed up/slow down slide duration) */
#define TILE_ANIMATION_DURATION 0.18f
```

---

## 4. Operational Workflow & Execution Flow

### System Execution Lifecycle

```
[Main Loop Setup]
       │
       ▼
   app_init() ───► Initialize seeds, queues, and start on GAME_TITLE screen.
       │
       ▼
 ┌► [Frame Loop] (60 FPS)
 │     ├─► input_sample()          : Read user keystrokes into GameInput
 │     ├─► app_frame()             : Step state machine, run simulation/animation updates
 │     ├─► debug_update()          : Update frame timings and metrics
 │     └─► BeginDrawing()
 │           ├─► render_frame()    : Draw title, interpolated tiles, or overlays
 │           └─► debug_draw()      : Draw debug overlay (if toggled via F3)
 │         EndDrawing()
 └───  │
       ▼
  app_shutdown() ──► Clean up simulation resources upon exit.
```

---

## 5. Module API Specification & Function Signatures

### 5.1 Simulation Module (`simulation.h`)

Handles pure game board logic and board state transitions.

* `void sim_begin_match(SimulationState *state, uint64_t seed, EventQueue *events)`
  * Resets the board state, seeds the internal PRNG, executes a random-walk shuffle guaranteed to preserve solvability, and emits `EVT_MATCH_STARTED`.
* `bool sim_can_move(const SimulationState *state, Direction direction)`
  * Evaluates whether moving a tile in the specified direction into the empty slot is a valid move.
* `bool sim_move(SimulationState *state, Direction direction, MoveResult *result, EventQueue *events)`
  * Performs a discrete state swap on the board, increments move counters, tests for puzzle completion, and logs `EVT_TILE_SLID` (and `EVT_MATCH_ENDED` if solved).
* `bool sim_check_solved(const SimulationState *state)`
  * Evaluates if tiles $1$ through $15$ are arranged sequentially with the empty slot at index $15$.
* `void sim_shutdown(SimulationState *state)`
  * Zero-clears the simulation instance on teardown.

---

### 5.2 Animation Module (`animation.h`)

Tracks frame time progress for tile sliding.

* `void animation_reset(TileAnimation *animation)`
  * Resets animation active state and elapsed counters to zero.
* `void animation_start(TileAnimation *animation, const MoveResult *move, float duration)`
  * Begins tracking a transition for `move->tile_value` from `move->from_index` to `move->to_index`.
* `void animation_update(TileAnimation *animation, float frame_time)`
  * Advances elapsed time by `frame_time` and deactivates the animation when complete.
* `float animation_progress(const TileAnimation *animation)`
  * Computes smoothstep progress value in range $[0.0, 1.0]$.

---

### 5.3 Event Queue Subsystem (`events.h`)

Ring buffer memory structure for storing historical simulation events.

* `void event_queue_clear(EventQueue *queue)`
  * Resets internal head writing counter to 0.
* `void event_queue_push(EventQueue *queue, const SimEvent *event)`
  * Writes an event to `queue->events[head & MASK]` and increments head.
* `uint64_t event_queue_total(const EventQueue *queue)`
  * Returns total number of written events across the lifetime of the queue.
* `bool event_queue_recent(const EventQueue *queue, uint32_t back, SimEvent *out)`
  * Fetches an event relative to the latest written head offset.
* `void event_cursor_sync(const EventQueue *queue, EventCursor *cursor)`
  * Synchronizes an external reader cursor to the current queue head.
* `bool event_cursor_read(const EventQueue *queue, EventCursor *cursor, SimEvent *out)`
  * Non-destructive reading helper for cursor tracking. Computes buffer drop rate if overruns occur.
* `const char *event_type_name(EventType type)`
  * Returns string literal representation of an event type for logging/UI (`"MATCH_START"`, `"TILE_SLID"`, `"MATCH_END"`).

---

### 5.4 Application & Controller Module (`app.h`)

Top-level state machine manager.

* `void app_init(App *app, uint64_t base_seed)`
  * Initializes state structures, initial seeds, and status strings.
* `void app_shutdown(App *app)`
  * Shutdown handler for the app container.
* `void app_frame(App *app, const GameInput *input, float frame_time)`
  * Main logic pump: updates input states, updates active animations, blocks rapid inputs during animation sliding, and issues simulation steps.
* `AppView app_view(const App *app)`
  * Constructs a read-only View struct (`AppView`) passed directly to rendering functions.

---

### 5.5 Pseudo-Random Number Generator (`rng.h`)

SplitMix64 implementation for reproducible seed generation.

* `uint64_t rng_splitmix64(uint64_t x)`
  * Pure function producing a 64-bit splitmix hash.
* `void rng_seed(Rng *rng, uint64_t seed, uint64_t stream)`
  * Seeds the PRNG state using `rng_splitmix64`.
* `uint64_t rng_next_u64(Rng *rng)`
  * Generates next pseudo-random 64-bit integer.
* `uint32_t rng_range(Rng *rng, uint32_t min, uint32_t max)`
  * Helper returning uniform integer between `min` and `max` inclusive.

---

### 5.6 Rendering & Debug Modules (`render.h`, `debug.h`, `input.h`, `log.h`)

* `void render_frame(const AppView *view)`
  * Master render call drawing boards, static tiles, and interpolated animated tiles.
* `void debug_init(DebugState *debug)`
  * Initializes debugging state metrics.
* `void debug_update(DebugState *debug, float raw_frame_time)`
  * Recalculates FPS and frame latency metrics.
* `void debug_draw(const DebugState *debug, const AppView *view, const EventQueue *events)`
  * Displays transparent debug overlay showing move count, current seed, and recent events.
* `void input_sample(GameInput *input)`
  * Queries Raylib hardware state (`IsKeyPressed`) and populates input struct flags.
* `void log_init(void)` / `void log_enable(LogCategory category)` / `void log_message(...)`
  * Internal conditional stdout logger partitioned into `GAME`, `SIM`, and `RENDER` streams.

---

## 6. Integration & Usage Examples

### Example 1: Direct Headless Simulation Execution
Running a simulation match without rendering, useful for automated testing or AI solver evaluation:

```c
#include "simulation.h"
#include "events.h"
#include <stdio.h>

int main(void) {
    SimulationState sim;
    EventQueue events;
    MoveResult result;

    event_queue_clear(&events);
    
    /* Initialize deterministic match with explicit seed */
    sim_begin_match(&sim, 0x1234567887654321ULL, &events);
    printf("Initial empty index: %u\n", sim.empty_index);

    /* Try making a move */
    if (sim_can_move(&sim, DIR_UP)) {
        sim_move(&sim, DIR_UP, &result, &events);
        printf("Moved tile %u from index %u to %u\n", 
               result.tile_value, result.from_index, result.to_index);
    } else {
        printf("DIR_UP move is not valid in current board layout.\n");
    }

    sim_shutdown(&sim);
    return 0;
}
```

### Example 2: Reading Events with EventCursor
Tracking tile movement events sequentially using an independent cursor:

```c
#include "events.h"
#include <stdio.h>

void print_new_events(const EventQueue *queue, EventCursor *cursor) {
    SimEvent event;
    while (event_cursor_read(queue, cursor, &event)) {
        printf("[Event %s] Move Count: %u | Tile: %u (%u -> %u)\n",
               event_type_name(event.type),
               event.move_count,
               event.tile_value,
               event.from_index,
               event.to_index);
    }
    
    if (cursor->lost > 0) {
        printf("Warning: %u events lost due to buffer ring wrap-around.\n", cursor->lost);
        cursor->lost = 0;
    }
}
```



Cellular Automata & Grid Transformation Theory (Academic & Applied)

"Cellular Automata Modeling of Physical Systems" (Standard computer science literature on grid state transitions).

Key Concepts to Research: Neighborhood functions (Von Neumann vs. Moore neighborhoods) which govern how a tile inspects its adjacent tiles (find_source_index) to resolve rule interactions like merging, pushing, or transforming.

