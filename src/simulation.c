#include "simulation.h"
#include "rng.h"
#include <string.h>

static bool check_solved(const SimulationState *sim) {
    uint8_t expected = 1;
    for (int r = 0; r < BOARD_SIZE; ++r) {
        for (int c = 0; c < BOARD_SIZE; ++c) {
            if (r == BOARD_SIZE - 1 && c == BOARD_SIZE - 1) {
                if (sim->board[r][c] != 0) return false;
            } else {
                if (sim->board[r][c] != expected++) return false;
            }
        }
    }
    return true;
}

static bool execute_slide(SimulationState *sim, int dr, int dc, int *moved_val) {
    int target_r = sim->empty_row + dr;
    int target_c = sim->empty_col + dc;

    if (target_r < 0 || target_r >= BOARD_SIZE || target_c < 0 || target_c >= BOARD_SIZE) {
        return false;
    }

    *moved_val = sim->board[target_r][target_c];
    sim->board[sim->empty_row][sim->empty_col] = *moved_val;
    sim->board[target_r][target_c] = 0;

    sim->empty_row = (uint8_t)target_r;
    sim->empty_col = (uint8_t)target_c;
    sim->move_count++;

    return true;
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
            state->board[r][c] = val++;
        }
    }
    state->board[BOARD_SIZE - 1][BOARD_SIZE - 1] = 0;
    state->empty_row = BOARD_SIZE - 1;
    state->empty_col = BOARD_SIZE - 1;

    // Guaranteed solvable shuffle via random valid moves
    int moves_done = 0;
    while (moves_done < 250) {
        int dir = (int)(rng_next_u32(&state->rng) % 4);
        int dr = 0, dc = 0;
        if (dir == 0) dr = -1;
        else if (dir == 1) dr = 1;
        else if (dir == 2) dc = -1;
        else if (dir == 3) dc = 1;

        int dummy = 0;
        if (execute_slide(state, dr, dc, &dummy)) {
            moves_done++;
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
    (void)dt;
    if (state->match_over) return;

    state->tick++;

    int dr = 0, dc = 0;
    if      (input->up)    { dr = 1;  dc = 0; }
    else if (input->down)  { dr = -1; dc = 0; }
    else if (input->left)  { dr = 0;  dc = 1; }
    else if (input->right) { dr = 0;  dc = -1; }

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
    }
}
