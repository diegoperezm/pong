#ifndef LOG_H
#define LOG_H

#include <stdint.h>

#define LOG_CATEGORIES(X)          \
    X(PUZZLE_LOG_GAME,       "GAME")   \
    X(PUZZLE_LOG_INPUT,      "INPUT")  \
    X(PUZZLE_LOG_SIMULATION, "SIM")    \
    X(PUZZLE_LOG_RENDER,     "RENDER")

#define LOG_CATEGORY_ENUM(category, name) category,

typedef enum {
    LOG_CATEGORIES(LOG_CATEGORY_ENUM)
    LOG_COUNT
} LogCategory;

#undef LOG_CATEGORY_ENUM

void log_init(void);
void log_enable(LogCategory category);
void log_disable(LogCategory category);
int log_is_enabled(LogCategory category);
void log_message(LogCategory category, const char *file, int line, const char *format, ...);

#define LOG_GAME(...)      log_message(PUZZLE_LOG_GAME, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_SIMULATION(...) log_message(PUZZLE_LOG_SIMULATION, __FILE__, __LINE__, __VA_ARGS__)

#endif
