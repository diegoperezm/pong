#include "input.h"
#include "raylib.h"


void input_sample(GameInput *input)
{
  input->up    = IsKeyDown(KEY_UP);
  input->down  = IsKeyDown(KEY_DOWN);
  input->pause = IsKeyPressed(KEY_ESCAPE);
  input->start = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE); 
};




