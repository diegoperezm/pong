#include "animation.h"
#include "app.h"
#include "config.h"
#include "simulation.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static Direction opposite(Direction direction)
{
    switch (direction) {
        case DIR_UP: return DIR_DOWN;
        case DIR_DOWN: return DIR_UP;
        case DIR_LEFT: return DIR_RIGHT;
        case DIR_RIGHT: return DIR_LEFT;
        default: return DIR_NONE;
    }
}

static void test_shuffle_is_a_permutation(void)
{
    SimulationState sim;
    bool seen[TOTAL_TILES] = { false };
    int i;

    sim_begin_match(&sim, 0x12345678ULL, NULL);
    for (i = 0; i < TOTAL_TILES; ++i) {
        uint8_t value = sim.board[i];
        assert(value < TOTAL_TILES);
        assert(!seen[value]);
        seen[value] = true;
        if (value == 0) assert(sim.empty_index == (uint8_t)i);
    }
    for (i = 0; i < TOTAL_TILES; ++i) assert(seen[i]);
    assert(!sim_check_solved(&sim));
    assert(sim.move_count == 0);
}

static void test_many_shuffles_are_valid_and_deterministic(void)
{
    for (uint64_t seed = 0; seed < 1000; ++seed) {
        SimulationState a;
        SimulationState b;
        bool seen[TOTAL_TILES] = { false };
        int i;

        sim_begin_match(&a, seed, NULL);
        sim_begin_match(&b, seed, NULL);
        assert(memcmp(a.board, b.board, sizeof(a.board)) == 0);
        assert(a.empty_index == b.empty_index);
        assert(!sim_check_solved(&a));

        for (i = 0; i < TOTAL_TILES; ++i) {
            uint8_t value = a.board[i];
            assert(value < TOTAL_TILES);
            assert(!seen[value]);
            seen[value] = true;
            if (value == 0) assert(a.empty_index == (uint8_t)i);
        }
        for (i = 0; i < TOTAL_TILES; ++i) assert(seen[i]);
    }
}

static void test_move_and_reverse_restore_board(void)
{
    SimulationState sim;
    uint8_t original[TOTAL_TILES];
    Direction direction = DIR_NONE;
    MoveResult move;
    int i;

    sim_begin_match(&sim, 987654321ULL, NULL);
    memcpy(original, sim.board, sizeof(original));
    for (i = DIR_UP; i <= DIR_RIGHT; ++i) {
        if (sim_can_move(&sim, (Direction)i)) {
            direction = (Direction)i;
            break;
        }
    }
    assert(direction != DIR_NONE);
    assert(sim_move(&sim, direction, &move, NULL));
    assert(sim.move_count == 1);
    assert(sim.board[move.from_index] == 0);
    assert(sim.board[move.to_index] == move.tile_value);
    assert(sim.empty_index == move.from_index);
    assert(sim_move(&sim, opposite(direction), NULL, NULL));
    assert(memcmp(original, sim.board, sizeof(original)) == 0);
    assert(sim.move_count == 2);
}

static void test_invalid_move_does_not_mutate_state(void)
{
    SimulationState sim = {0};
    uint8_t original[TOTAL_TILES];

    for (int i = 0; i < TOTAL_TILES - 1; ++i) sim.board[i] = (uint8_t)(i + 1);
    sim.board[TOTAL_TILES - 1] = 0;
    sim.empty_index = TOTAL_TILES - 1;
    memcpy(original, sim.board, sizeof(original));

    assert(!sim_can_move(&sim, DIR_UP));
    assert(!sim_move(&sim, DIR_UP, NULL, NULL));
    assert(memcmp(original, sim.board, sizeof(original)) == 0);
    assert(sim.move_count == 0);
}

static void test_solving_is_immediate_and_emits_events(void)
{
    SimulationState sim = {0};
    EventQueue events;
    SimEvent event;

    event_queue_clear(&events);
    for (int i = 0; i < TOTAL_TILES - 2; ++i) sim.board[i] = (uint8_t)(i + 1);
    sim.board[TOTAL_TILES - 2] = 0;
    sim.board[TOTAL_TILES - 1] = TOTAL_TILES - 1;
    sim.empty_index = TOTAL_TILES - 2;

    assert(sim_move(&sim, DIR_LEFT, NULL, &events));
    assert(sim.is_solved);
    assert(sim.match_over);
    assert(sim_check_solved(&sim));
    assert(sim.move_count == 1);
    assert(event_queue_recent(&events, 0, &event));
    assert(event.type == EVT_MATCH_ENDED);
    assert(event.move_count == 1);
}

static void test_animation_is_separate_from_simulation(void)
{
    App app;
    GameInput input = {0};
    Direction direction = DIR_NONE;
    uint32_t moves_before;

    app_init(&app, 42);
    input.start = true;
    app_frame(&app, &input, 1.0f);
    assert(app.mode == GAME_PLAYING);

    memset(&input, 0, sizeof(input));
    for (int i = DIR_UP; i <= DIR_RIGHT; ++i) {
        if (sim_can_move(&app.sim, (Direction)i)) {
            direction = (Direction)i;
            break;
        }
    }
    assert(direction != DIR_NONE);
    if (direction == DIR_UP) input.up = true;
    if (direction == DIR_DOWN) input.down = true;
    if (direction == DIR_LEFT) input.left = true;
    if (direction == DIR_RIGHT) input.right = true;

    moves_before = app.sim.move_count;
    app_frame(&app, &input, 0.016f);
    assert(app.sim.move_count == moves_before + 1);
    assert(app.animation.active);

    memset(&input, 0, sizeof(input));
    app_frame(&app, &input, TILE_ANIMATION_DURATION);
    app_frame(&app, &input, TILE_ANIMATION_DURATION);
    assert(!app.animation.active);
    assert(app.sim.move_count == moves_before + 1);
    app_shutdown(&app);
}

int main(void)
{
    test_shuffle_is_a_permutation();
    test_many_shuffles_are_valid_and_deterministic();
    test_move_and_reverse_restore_board();
    test_invalid_move_does_not_mutate_state();
    test_solving_is_immediate_and_emits_events();
    test_animation_is_separate_from_simulation();
    puts("All puzzle core tests passed.");
    return 0;
}
