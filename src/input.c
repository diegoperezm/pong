#include "input.h"
#include "raylib.h"

void input_sample(GameInput *input) {
    /* Continuous movement checks (support both Arrows and WASD) */
    input->up    = IsKeyDown(KEY_UP) || IsKeyDown(KEY_W);
    input->down  = IsKeyDown(KEY_DOWN) || IsKeyDown(KEY_S);
    input->left  = IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_A);
    input->right = IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_D);

    /* Edge-triggered action keys */
    input->action1 = IsKeyPressed(KEY_J) || IsKeyPressed(KEY_Z);
    input->action2 = IsKeyPressed(KEY_K) || IsKeyPressed(KEY_X);

    /* System controls (edge-triggered: true for exactly one frame) */
    input->start        = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    input->pause        = IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE);
    input->step         = IsKeyPressed(KEY_N); /* Useful for stepping frames when paused */
    input->debug_toggle = IsKeyPressed(KEY_F3);
}
