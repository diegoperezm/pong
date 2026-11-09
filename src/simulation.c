#include "simulation.h"
#include "config.h"
#include "rng.h"

#include <assert.h>
#include <string.h>

static bool direction_delta(Direction direction, int *dr, int *dc)
{
    switch (direction) {
        case DIR_UP:    *dr = -1; *dc =  0; return true;
        case DIR_DOWN:  *dr =  1; *dc =  0; return true;
        case DIR_LEFT:  *dr =  0; *dc = -1; return true;
        case DIR_RIGHT: *dr =  0; *dc =  1; return true;
        default: return false;
    }
}

static Direction opposite_direction(Direction direction)
{
    switch (direction) {
        case DIR_UP:    return DIR_DOWN;
        case DIR_DOWN:  return DIR_UP;
        case DIR_LEFT:  return DIR_RIGHT;
        case DIR_RIGHT: return DIR_LEFT;
        default:        return DIR_NONE;
    }
}

/* Direction describes the direction in which the tile moves. */
static bool find_source_index(const SimulationState *state, Direction direction,
                              uint8_t *source_index)
{
    int dr = 0;
    int dc = 0;
    int empty_row;
    int empty_col;
    int source_row;
    int source_col;

    if (!state || !source_index || !direction_delta(direction, &dr, &dc))
        return false;

    empty_row = state->empty_index / BOARD_SIZE;
    empty_col = state->empty_index % BOARD_SIZE;
    source_row = empty_row - dr;
    source_col = empty_col - dc;

    if (source_row < 0 || source_row >= BOARD_SIZE ||
        source_col < 0 || source_col >= BOARD_SIZE)
        return false;

    *source_index = (uint8_t)(source_row * BOARD_SIZE + source_col);
    return state->board[*source_index] != 0;
}

bool sim_can_move(const SimulationState *state, Direction direction)
{
    uint8_t source_index;
    if (!state || state->match_over || state->is_solved) return false;
    return find_source_index(state, direction, &source_index);
}

/* This is the only operation that changes the board layout. No time or position data. */
static bool apply_swap(SimulationState *state, Direction direction, MoveResult *result)
{
    uint8_t source_index;
    uint8_t destination_index;
    uint8_t tile_value;

    if (!find_source_index(state, direction, &source_index)) return false;

    destination_index = state->empty_index;
    tile_value = state->board[source_index];
    assert(tile_value >= 1 && tile_value < TOTAL_TILES);

    state->board[destination_index] = tile_value;
    state->board[source_index] = 0;
    state->empty_index = source_index;

    if (result) {
        result->tile_value = tile_value;
        result->from_index = source_index;
        result->to_index = destination_index;
    }
    return true;
}

bool sim_check_solved(const SimulationState *state)
{
    int i;
    if (!state) return false;
    for (i = 0; i < TOTAL_TILES - 1; ++i) {
        if (state->board[i] != (uint8_t)(i + 1)) return false;
    }
    return state->board[TOTAL_TILES - 1] == 0;
}

bool sim_move(SimulationState *state, Direction direction, MoveResult *result,
              EventQueue *events)
{
    MoveResult move;
    SimEvent event;

    if (!state || state->match_over || state->is_solved) return false;
    if (!apply_swap(state, direction, &move)) return false;

    state->move_count++;
    state->is_solved = sim_check_solved(state);
    state->match_over = state->is_solved;

    if (result) *result = move;

    event = (SimEvent){
        .move_count = state->move_count,
        .type = EVT_TILE_SLID,
        .tile_value = move.tile_value,
        .from_index = move.from_index,
        .to_index = move.to_index
    };
    event_queue_push(events, &event);

    if (state->match_over) {
        event.type = EVT_MATCH_ENDED;
        event_queue_push(events, &event);
    }
    return true;
}

static void shuffle_board(SimulationState *state)
{
    Direction directions[4] = { DIR_UP, DIR_DOWN, DIR_LEFT, DIR_RIGHT };
    Direction previous = DIR_NONE;
    int step;

    for (step = 0; step < SHUFFLE_MOVE_COUNT; ++step) {
        Direction candidates[4];
        int candidate_count = 0;
        int i;

        for (i = 0; i < 4; ++i) {
            Direction direction = directions[i];
            uint8_t source_index;
            if (!find_source_index(state, direction, &source_index)) continue;
            if (direction == opposite_direction(previous)) continue;
            candidates[candidate_count++] = direction;
        }

        /* At a corner/edge there is always a legal move. If excluding the
           reverse direction removed all options, allow it rather than stall. */
        if (candidate_count == 0) {
            for (i = 0; i < 4; ++i) {
                uint8_t source_index;
                if (find_source_index(state, directions[i], &source_index))
                    candidates[candidate_count++] = directions[i];
            }
        }

        if (candidate_count > 0) {
            Direction chosen = candidates[rng_range(&state->rng, 0, (uint32_t)candidate_count - 1u)];
            (void)apply_swap(state, chosen, NULL);
            previous = chosen;
        }
    }
}

void sim_begin_match(SimulationState *state, uint64_t seed, EventQueue *events)
{
    int i;
    if (!state) return;

    memset(state, 0, sizeof(*state));
    state->seed = seed;
    rng_seed(&state->rng, seed, 1);

    for (i = 0; i < TOTAL_TILES - 1; ++i)
        state->board[i] = (uint8_t)(i + 1);
    state->board[TOTAL_TILES - 1] = 0;
    state->empty_index = TOTAL_TILES - 1;

    shuffle_board(state);

    /* A random walk is solvable by construction. Keep the explicit check as
       a guard against future changes to the shuffle implementation. */
    if (sim_check_solved(state)) {
        for (i = 0; i < 4; ++i) {
            uint8_t source_index;
            if (find_source_index(state, (Direction)(i + 1), &source_index)) {
                (void)apply_swap(state, (Direction)(i + 1), NULL);
                break;
            }
        }
    }

    state->move_count = 0;
    state->is_solved = false;
    state->match_over = false;

    {
        SimEvent event = { .move_count = 0, .type = EVT_MATCH_STARTED };
        event_queue_push(events, &event);
    }
}

void sim_shutdown(SimulationState *state)
{
    if (state) memset(state, 0, sizeof(*state));
}
