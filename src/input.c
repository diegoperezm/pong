#include "input.h"
#include "raylib.h"

void input_sample(GameInput *input)
{
  input->up    = IsKeyDown(KEY_UP);
  input->down  = IsKeyDown(KEY_DOWN);

  /* Edge-triggered: true for exactly the one frame the key goes down. */
  input->start = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE);
  input->pause = IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE);

  input->replay       = IsKeyPressed(KEY_F6);
  input->step         = IsKeyPressed(KEY_PERIOD);
  input->seek_back    = IsKeyPressed(KEY_LEFT_BRACKET);
  input->seek_fwd     = IsKeyPressed(KEY_RIGHT_BRACKET);
  input->debug_toggle = IsKeyPressed(KEY_F3);
}
