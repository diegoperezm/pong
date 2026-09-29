#include "input.h"
#include "raylib.h"

void input_sample(GameInput *input)
{
  input->up    = IsKeyDown(KEY_UP);
  input->down  = IsKeyDown(KEY_DOWN);
  input->start = IsKeyPressed(KEY_ENTER) || IsKeyPressed(KEY_SPACE); 
  
  // CORRECCIÓN: Botón de pausa usando IsKeyPressed (evalúa una vez por pulsación)
  input->pause = IsKeyPressed(KEY_P) || IsKeyPressed(KEY_ESCAPE);
};




