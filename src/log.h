#ifndef LOG_H
#define LOG_H

#include <stdint.h>

#define LOG_CATEGORIES(X)              \
X(PONG_LOG_GAME,       "GAME")         \
X(PONG_LOG_INPUT,      "INPUT")        \
X(PONG_LOG_SIMULATION, "SIMULATION")   \
X(PONG_LOG_COLLISION,  "COLLISION")    \
X(PONG_LOG_ENTITY,     "ENTITY")       \
X(PONG_LOG_AI,         "AI")           \
X(PONG_LOG_RENDER,     "RENDER")       \
X(PONG_LOG_DEBUG,      "DEBUG")        \


#define LOG_CATEGORY_ENUM(category, name) category,

typedef enum
{
LOG_CATEGORIES(LOG_CATEGORY_ENUM)
LOG_COUNT
} LogCategory;

#undef LOG_CATEGORY_ENUM

#define LOG_CATEGORY_NAME(category, name) [category] = name,

#undef LOG_CATEGORY_NAME

extern const char* log_category_name[LOG_COUNT];

void log_init(void);

void log_enable(LogCategory category);
void log_disable(LogCategory category);
int log_is_enabled(LogCategory category);

void log_message(
  LogCategory category,
  const char* file,
  int line,
  const char* format,
  ...
);

#define LOG_GAME(...)      \
 log_message(PONG_LOG_GAME, __FILE__, __LINE__, __VA_ARGS__)

#define LOG_INPUT(...)      \
 log_message(PONG_LOG_INPUT, __FILE__, __LINE__, __VA_ARGS__)

#define LOG_SIMULATION(...) \
 log_message(PONG_LOG_SIMULATION, __FILE__, __LINE__, __VA_ARGS__)

#define LOG_COLLISION(...)  \
 log_message(PONG_LOG_COLLISION, __FILE__, __LINE__, __VA_ARGS__)

#define LOG_ENTITY(...)     \
 log_message(PONG_LOG_ENTITY, __FILE__, __LINE__, __VA_ARGS__)

#define LOG_AI(...)         \
 log_message(PONG_LOG_AI, __FILE__, __LINE__, __VA_ARGS__)

#define LOG_RENDER(...)     \
 log_message(PONG_LOG_RENDER, __FILE__, __LINE__, __VA_ARGS__)

#define LOG_DEBUG(...)      \
 log_message(PONG_LOG_DEBUG, __FILE__, __LINE__, __VA_ARGS__)

#endif
