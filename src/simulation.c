#include "simulation.h"
#include "rng.h"
#include <string.h>
#include <math.h>
#include <assert.h>

static bool check_solved(const SimulationState *sim) {
    uint8_t expected = 1;
    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            if (r == BOARD_SIZE - 1 && c == BOARD_SIZE - 1) {
                if (sim->logical_board[r][c] != 0) return false;
            } else {
                if (sim->logical_board[r][c] != expected++) return false;
            }
        }
    }
    return true;
}

static bool all_tiles_settled(const SimulationState *sim) {
    for (int i = 0; i < TOTAL_TILES - 1; ++i) {
        if (sim->tiles[i].is_moving)
            return false;
    }
    return true;
}

static bool any_tile_moving(const SimulationState *sim) {
    return !all_tiles_settled(sim);
}

static bool execute_slide(SimulationState *sim, int dr, int dc, int *moved_val) {
    // Physical movement policy: do not accept a new move while any tile is still sliding
    if (any_tile_moving(sim)) {
        return false;
    }

    int target_r = sim->empty_row + dr;
    int target_c = sim->empty_col + dc;

    if (target_r < 0 || target_r >= BOARD_SIZE || target_c < 0 || target_c >= BOARD_SIZE) {
        return false;
    }

    *moved_val = sim->logical_board[target_r][target_c];
    assert(*moved_val >= 1 && *moved_val < TOTAL_TILES);

    sim->logical_board[sim->empty_row][sim->empty_col] = *moved_val;
    sim->logical_board[target_r][target_c] = 0;

    int tile_idx = *moved_val - 1;
    sim->tiles[tile_idx].target.row = (float)sim->empty_row;
    sim->tiles[tile_idx].target.col = (float)sim->empty_col;
    sim->tiles[tile_idx].is_moving = true;

    sim->empty_row = (uint8_t)target_r;
    sim->empty_col = (uint8_t)target_c;
    sim->move_count++;

    return true;
}

static void update_physics(SimulationState *sim, float dt, EventQueue *events) {
    for (int i = 0; i < TOTAL_TILES - 1; ++i) {
        TileBody *tile = &sim->tiles[i];
        if (!tile->is_moving) continue;

        float dr = tile->target.row - tile->position.row;
        float dc = tile->target.col - tile->position.col;
        float dist = sqrtf(dr * dr + dc * dc);

        if (dist < 0.01f) {
            tile->position = tile->target;
            tile->velocity = (GridPosition){ 0.0f, 0.0f };
            tile->is_moving = false;

            if (events) {
                SimEvent e = {
                    .tick = sim->tick,
                    .type = EVT_TILE_SETTLED,
                    .tile_value = tile->value
                };
                event_queue_push(events, &e);
            }
        } else {
            float factor = 1.0f - expf(-TILE_SLIDE_SPEED * dt);
            tile->position.row += dr * factor;
            tile->position.col += dc * factor;
            tile->velocity.row = dr * TILE_SLIDE_SPEED;
            tile->velocity.col = dc * TILE_SLIDE_SPEED;
        }
    }
}

void sim_shutdown(SimulationState *state) {
    memset(state, 0, sizeof(*state));
}

void sim_begin_match(SimulationState *state, uint64_t seed, EventQueue *events) {
    memset(state, 0, sizeof(*state));
    state->seed = seed;
    rng_seed(&state->rng, seed, 1);

    uint8_t val = 1;
    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            state->logical_board[r][c] = val;
            if (val <= TOTAL_TILES - 1) {
                int idx = val - 1;
                state->tiles[idx].value = val;
                state->tiles[idx].position = (GridPosition){ (float)r, (float)c };
                state->tiles[idx].target   = (GridPosition){ (float)r, (float)c };
                state->tiles[idx].velocity = (GridPosition){ 0.0f, 0.0f };
                state->tiles[idx].is_moving = false;
            }
            val++;
        }
    }
    state->logical_board[BOARD_SIZE - 1][BOARD_SIZE - 1] = 0;
    state->empty_row = BOARD_SIZE - 1;
    state->empty_col = BOARD_SIZE - 1;

    // Guaranteed solvable shuffle via random valid moves, avoiding immediate reverse moves
    int moves_done = 0;
    int last_dir = -1;
    while (moves_done < 250) {
        int dir = (int)(rng_next_u64(&state->rng) % 4);
        // Avoid reversing previous move
        if ((dir == 0 && last_dir == 1) || (dir == 1 && last_dir == 0) ||
            (dir == 2 && last_dir == 3) || (dir == 3 && last_dir == 2)) {
            dir = (dir + 1) % 4;
        }

        int dr = 0, dc = 0;
        if (dir == 0) dr = -1;
        else if (dir == 1) dr = 1;
        else if (dir == 2) dc = -1;
        else if (dir == 3) dc = 1;

        int moved_val = 0;
        if (execute_slide(state, dr, dc, &moved_val)) {
            moves_done++;
            last_dir = dir;
            // FIX: Reset is_moving so shuffle isn't blocked by physical movement flags
            if (moved_val >= 1 && moved_val < TOTAL_TILES) {
                state->tiles[moved_val - 1].is_moving = false;
            }
        }
    }



    // Ensure the shuffle did not result in an already solved board
    if (check_solved(state)) {
        // Force one extra valid slide if solved
        int dr = (state->empty_row > 0) ? -1 : 1;
        int dummy = 0;
        execute_slide(state, dr, 0, &dummy);
    }

    // Snap physical positions to post-shuffle logical grid
    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            uint8_t v = state->logical_board[r][c];
            if (v != 0) {
                int idx = v - 1;
                state->tiles[idx].position = (GridPosition){ (float)r, (float)c };
                state->tiles[idx].target   = (GridPosition){ (float)r, (float)c };
                state->tiles[idx].is_moving = false;
            }
        }
    }

    state->move_count = 0;
    state->is_solved = false;
    state->match_over = false;

    if (events) {
        SimEvent e = { .tick = 0, .type = EVT_MATCH_STARTED };
        event_queue_push(events, &e);
    }
}

void sim_step(SimulationState *state, const TickInput *input, float dt, EventQueue *events) {
    if (state->match_over) return;

    state->tick++;

    // Only accept input if puzzle is not logically solved yet
    if (!state->is_solved) {
        int dr = 0, dc = 0;
        // Arrow keys direction convention: tile moves towards empty space
        if (input->move_dir == DIR_UP)    { dr = 1;  dc = 0; }
        if (input->move_dir == DIR_DOWN)  { dr = -1; dc = 0; }
        if (input->move_dir == DIR_LEFT)  { dr = 0;  dc = 1; }
        if (input->move_dir == DIR_RIGHT) { dr = 0;  dc = -1; }

        if (dr != 0 || dc != 0) {
            int moved_val = 0;
            if (execute_slide(state, dr, dc, &moved_val)) {
                if (events) {
                    SimEvent e = {
                        .tick = state->tick,
                        .type = EVT_TILE_SLID,
                        .tile_value = moved_val,
                        .score_player = (int)state->move_count
                    };
                    event_queue_push(events, &e);
                }

                if (check_solved(state)) {
                    state->is_solved = true;
                }
            }
        }
    }

    update_physics(state, dt, events);

    // Separating logical completion from physical completion
    if (state->is_solved && all_tiles_settled(state)) {
        state->match_over = true;
        if (events) {
            SimEvent win_event = {
                .tick = state->tick,
                .type = EVT_MATCH_ENDED,
                .score_player = (int)state->move_count
            };
            event_queue_push(events, &win_event);
        }
    }
}
