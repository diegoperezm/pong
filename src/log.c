#include "log.h"
#include <stdio.h>
#include <stdarg.h>

static uint32_t enabled_categories;

#define LOG_CATEGORY_NAME(category, name) [category] = name,

static const char *log_category_names[LOG_COUNT] = {
    LOG_CATEGORIES(LOG_CATEGORY_NAME)
};

#undef LOG_CATEGORY_NAME

void log_init(void) { enabled_categories = 0; }
void log_enable(LogCategory c) { if ((unsigned)c < LOG_COUNT && c < 32) enabled_categories |= (1u << c); }
void log_disable(LogCategory c) { if ((unsigned)c < LOG_COUNT && c < 32) enabled_categories &= ~(1u << c); }

int log_is_enabled(LogCategory c) {
    if ((unsigned)c >= LOG_COUNT || c >= 32) return 0;
    return (enabled_categories & (1u << c)) != 0;
}

void log_message(LogCategory category, const char *file, int line, const char *format, ...) {
    if ((unsigned)category >= LOG_COUNT || !log_is_enabled(category)) return;
    
    printf("[%s] %s:%d: ", log_category_names[category], file, line);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
}
