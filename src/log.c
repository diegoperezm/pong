#include "log.h"
#include <stdio.h>
#include <stdarg.h>

static uint32_t enabled_categories;

void log_init(void) { enabled_categories = 0; }
void log_enable(LogCategory c) { if (c < LOG_COUNT) enabled_categories |= (1u << c); }
void log_disable(LogCategory c) { if (c < LOG_COUNT) enabled_categories &= ~(1u << c); }
int log_is_enabled(LogCategory c) { return (enabled_categories & (1u << c)) != 0; }

void log_message(LogCategory category, const char *file, int line, const char *format, ...) {
    if (!log_is_enabled(category)) return;
    printf("[%s] %s:%d: ", "LOG", file, line);
    va_list args;
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    printf("\n");
    fflush(stdout);
}
