#include "input.h"
#include "raylib.h"

void input_sample(GameInput *input)
{
  input->up    = IsKeyDown(KEY_UP);
  input->down  = IsKeyDown(KEY_DOWN);

  /* Edge-triggered: true for exactly the one frame the key goes down. */
  input->start        = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
  input->pause        = IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE);
  input->debug_toggle = IsKeyPressed(KEY_F3);
}
