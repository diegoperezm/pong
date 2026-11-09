#include "input.h"
#include "raylib.h"

void input_sample(GameInput *input)
{
    if (!input) return;
    input->up = IsKeyPressed(KEY_UP) || IsKeyPressed(KEY_W);
    input->down = IsKeyPressed(KEY_DOWN) || IsKeyPressed(KEY_S);
    input->left = IsKeyPressed(KEY_LEFT) || IsKeyPressed(KEY_A);
    input->right = IsKeyPressed(KEY_RIGHT) || IsKeyPressed(KEY_D);
    input->start = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
    input->pause = IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE);
    input->debug_toggle = IsKeyPressed(KEY_F3);
}
