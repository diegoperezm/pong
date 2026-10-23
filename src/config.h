#ifndef CONFIG_H
#define CONFIG_H

#define SCREEN_WIDTH          800
#define SCREEN_HEIGHT         600

#define SIM_HZ                60
#define SIM_DT                (1.0f / (float)SIM_HZ)
#define MAX_FRAME_TIME        0.25f  /* Spiral-of-death protection clamp */

#define MAX_ENTITIES          64
#define MAX_PARTICLES         256
#define MAX_POWERUPS          16

/* Debug Panel Layout Constants */
#define DEBUG_PANEL_X         10
#define DEBUG_PANEL_Y         10
#define DEBUG_PANEL_WIDTH     260
#define DEBUG_PANEL_HEIGHT    220
#define DEBUG_FONT_SIZE       10
#define DEBUG_LINE_HEIGHT     14
#define DEBUG_PADDING         8

#endif // CONFIG_H
