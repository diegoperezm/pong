#include "render.h"
#include "raylib.h"

static float
lerp_float(
  float a,
  float b,
  float t)
{
  return a + (b - a) * t;
}

// COURT
static void
draw_court(void)
{
  DrawRectangleLines(
  (int)COURT_LEFT,
  (int)COURT_TOP,
  (int)(COURT_RIGHT - COURT_LEFT),
  (int)(COURT_BOTTOM - COURT_TOP),
  WHITE
  );

  for (int y = (int)COURT_TOP; y < (int)COURT_BOTTOM; y += 20) {
   DrawRectangle(
     SCREEN_WIDTH / 2 - 2,
     y,
     4,
     10,
     WHITE
   );
 }
}

// PADDLES
static void
draw_paddles(
  const RenderSnapshot *previous,
  const RenderSnapshot *current,
  float alpha
)
{
  float player_x = lerp_float(
  previous->player_x,
  current->player_x,
  alpha
  );

  float player_y = lerp_float(
    previous->player_y,
    current->player_y,
    alpha
  );

  float enemy_x = lerp_float(
    previous->enemy_x,
    current->enemy_x,
    alpha
  );

  float enemy_y = lerp_float(
    previous->enemy_y,
    current->enemy_y,
    alpha
  );

  DrawRectangle(
    (int)player_x,
    (int)player_y,
    (int)PADDLE_WIDTH,
    (int)PADDLE_HEIGHT,
    WHITE
  );

  DrawRectangle(
    (int)enemy_x,
    (int)enemy_y,
    (int)PADDLE_WIDTH,
    (int)PADDLE_HEIGHT,
    WHITE
  );
}

// BALLS
// BALLS
static void
draw_balls(
  const RenderSnapshot *previous,
  const RenderSnapshot *current,
  float alpha
)
{
  for (int i = 0; i < MAX_BALLS; ++i) {
    if (!current->balls[i].active)
      continue;

    float x = current->balls[i].x;
    float y = current->balls[i].y;

    // Only interpolate if the slot was active previously AND belongs to the exact same generation
    if (previous->balls[i].active && previous->balls[i].generation == current->balls[i].generation) {
      x = lerp_float(
        previous->balls[i].x,
        current->balls[i].x,
        alpha
      );

      y = lerp_float(
        previous->balls[i].y,
        current->balls[i].y,
        alpha
      );
    }

    DrawRectangle(
      (int)x,
      (int)y,
      (int)BALL_SIZE,
      (int)BALL_SIZE,
      WHITE
    );
  }
}


// PARTICLES
static void
draw_particles(const RenderSnapshot *snapshot)
{
  for (int i = 0; i < MAX_PARTICLES; ++i) {
    if (!snapshot->particles[i].active)
    continue;


   float max_life = snapshot->particles[i].max_lifetime;
   float life = (max_life > 0.0f) ? (snapshot->particles[i].lifetime / max_life) : 0.0f;

   int alpha = (int)(life * 255.0f);

   if (alpha < 0) 
      alpha = 0;
    
   if (alpha > 255) 
      alpha = 255;
  
   Color color = {
      255,
      255,
      255,
     (unsigned char)alpha
   };
  
  DrawRectangle(
    (int)snapshot->particles[i].x,
    (int)snapshot->particles[i].y,
    (int)snapshot->particles[i].size,
    (int)snapshot->particles[i].size,
    color
  );
  }
}

// POWERUPS
static void
draw_powerups(const RenderSnapshot *snapshot)
{
  for (int i = 0; i < MAX_POWERUPS; ++i) {
   if (!snapshot->powerups[i].active)
     continue;
   
   Color color;
   
   if (snapshot->powerups[i].type == POWERUP_FAST_PADDLE)
     color = RED;
   else
     color = BLUE;
   
   DrawRectangle(
     (int)snapshot->powerups[i].x,
     (int)snapshot->powerups[i].y,
     (int)POWERUP_SIZE,
     (int)POWERUP_SIZE,
     color
   );
  }
}

// SCORE
static void
draw_score(const RenderSnapshot *snapshot)
{
  const char *p1_score = TextFormat("%d", snapshot->player_score);
  const char *p2_score = TextFormat("%d", snapshot->enemy_score);

  int p1_width = MeasureText(p1_score, 40);
  int p2_width = MeasureText(p2_score, 40);

  DrawText(p1_score, (SCREEN_WIDTH / 2) - 100 - (p1_width / 2), 30, 40, WHITE);
  DrawText(p2_score, (SCREEN_WIDTH / 2) + 100 - (p2_width / 2), 30, 40, WHITE);
}

// SCREENS
static void
draw_title(void)
{
  const char *title = "PONG";
  const char *prompt = "PRESS ENTER";

  DrawText(title, (SCREEN_WIDTH / 2) - (MeasureText(title, 40) / 2), 120, 40, WHITE);
  DrawText(prompt, (SCREEN_WIDTH / 2) - (MeasureText(prompt, 20) / 2), 180, 20, WHITE);
}

static void
draw_pause(void)
{
  const char *text = "PAUSED";
  DrawText(text, (SCREEN_WIDTH / 2) - (MeasureText(text, 25) / 2), 200, 25, WHITE);
}

static void
draw_game_over(const RenderSnapshot *snapshot)
{
  const char *text = snapshot->winner == 1 ? "YOU WIN" : "YOU LOSE";
  const char *score_text = TextFormat("%d - %d", snapshot->player_score, snapshot->enemy_score);
  const char *prompt = "PRESS ENTER";

  DrawText(text, (SCREEN_WIDTH / 2) - (MeasureText(text, 30) / 2), 120, 30, WHITE);
  DrawText(score_text, (SCREEN_WIDTH / 2) - (MeasureText(score_text, 20) / 2), 170, 20, WHITE);
  DrawText(prompt, (SCREEN_WIDTH / 2) - (MeasureText(prompt, 20) / 2), 210, 20, WHITE);
}

// LIFECYCLE
void
render_init(void)
{
}

void
render_shutdown(void)
{
}

// MAIN RENDER
void
render_frame(
  const RenderSnapshot *previous,
  const RenderSnapshot *current,
  float alpha
)
{
  BeginDrawing();
  ClearBackground(BLACK);


//    if (debug && sim_state) {
 //       debug_draw(debug, sim_state);
  //  }
   

  switch (current->mode) {
    case GAME_TITLE:
      draw_title();
      break;
    
    case GAME_PLAYING:
      draw_court();
      draw_powerups(current);
      draw_paddles(previous, current, alpha);
      draw_balls(previous, current, alpha);
      draw_particles(current);
      draw_score(current);
      break;
    
    case GAME_PAUSED:
      draw_court();
      draw_powerups(current);
      draw_paddles(previous, current, alpha);
      draw_balls(previous, current, alpha);
      draw_particles(current);
      draw_score(current);
      draw_pause();
      break;
    
    case GAME_OVER:
      draw_game_over(current);
      break;
 }
  
  EndDrawing();
}



