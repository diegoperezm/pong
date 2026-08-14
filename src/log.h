#ifndef LOG_H
#define LOG_H

typedef enum
{
 LOG_GAME,
 LOG_INPUT,
 LOG_SIMULATION,
 LOG_COLLISION,
 LOG_ENTITY,
 LOG_AI,
 LOG_RENDER,
 PONG_LOG_DEBUG,
 LOG_COUNT
} LogCategory;


void log_init(void);

void log_set_enabled(
  LogCategory category,
  int enabled);

int log_is_enabled(LogCategory category);

void log_message(
   LogCategory category,
   const char *file,
   int line,
   const char *format,
   ...
);

#define LOG_GAME(...) \
    log_message( \
        LOG_GAME, \
        __FILE__, \
        __LINE__, \
        __VA_ARGS__ \
    )

#define LOG_INPUT(...) \
    log_message( \
        LOG_INPUT, \
        __FILE__, \
        __LINE__, \
        __VA_ARGS__ \
    )

#define LOG_SIMULATION(...) \
    log_message( \
        LOG_SIMULATION, \
        __FILE__, \
        __LINE__, \
        __VA_ARGS__ \
    )

#define LOG_COLLISION(...) \
    log_message( \
        LOG_COLLISION, \
        __FILE__, \
        __LINE__, \
        __VA_ARGS__ \
    )

#define LOG_ENTITY(...) \
    log_message( \
        LOG_ENTITY, \
        __FILE__, \
        __LINE__, \
        __VA_ARGS__ \
    )

#define LOG_AI(...) \
    log_message( \
        LOG_AI, \
        __FILE__, \
        __LINE__, \
        __VA_ARGS__ \
    )

#define LOG_RENDER(...) \
    log_message( \
        LOG_RENDER, \
        __FILE__, \
        __LINE__, \
        __VA_ARGS__ \
    )

#define PONG_LOG_DEBUG(...) \
    log_message( \
        PONG_LOG_DEBUG, \
        __FILE__, \
        __LINE__, \
        __VA_ARGS__ \
    )
#endif

