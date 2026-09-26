#ifndef CONFIG_H
#define CONFIG_H

#define SCREEN_WIDTH       800
#define SCREEN_HEIGHT      800//450

// simulation runs independently of rendering
#define SIM_HZ             120
#define SIM_DT             (1.0f / (float)SIM_HZ)

#define MAX_FRAME_TIME     0.25
#define COURT_LEFT         20.0f
#define COURT_RIGHT        780.0f
#define COURT_TOP          20.0f
#define COURT_BOTTOM       430.0f

#define PADDLE_WIDTH       12.0f
#define PADDLE_HEIGHT      80.0f

#define PLAYER_SPEED       420.0f
#define AI_SPEED           300.0f

#define BALL_SIZE          10.0f

#define INITIAL_BALL_SPEED 350.0f
#define MAX_BALL_SPEED     700.0f

#define MAX_BALLS          3 // 32
#define INVALID_INDEX      0xFFFFFFFF
#define MAX_PARTICLES      512 
#define MAX_POWERUPS       16 
#define POWERUP_SIZE       18.0f 
#define POWERUP_INTERVAL   5.0f 
#define WINNING_SCORE      10 

// Particle Configuration Constants
#define PARTICLE_COUNT_ON_SCORE 20
#define PARTICLE_MIN_SPEED      50
#define PARTICLE_MAX_SPEED      180
#define PARTICLE_MIN_LIFETIME   250 // ms
#define PARTICLE_MAX_LIFETIME   350 // ms
#define PARTICLE_MIN_SIZE       2
#define PARTICLE_MAX_SIZE       5


#endif


