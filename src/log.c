#include "log.h"

#include <stdarg.h>
#include <stdio.h>

static uint32_t enabled_categories;

#define LOG_CATEGORY_NAME(category, name) [category] = name,
static const char *log_category_names[LOG_COUNT] = {
    LOG_CATEGORIES(LOG_CATEGORY_NAME)
};
#undef LOG_CATEGORY_NAME

void log_init(void) { enabled_categories = 0; }

void log_enable(LogCategory category)
{
    if ((unsigned)category < LOG_COUNT && (unsigned)category < 32u)
        enabled_categories |= (1u << (unsigned)category);
}

void log_disable(LogCategory category)
{
    if ((unsigned)category < LOG_COUNT && (unsigned)category < 32u)
        enabled_categories &= ~(1u << (unsigned)category);
}

int log_is_enabled(LogCategory category)
{
    if ((unsigned)category >= LOG_COUNT || (unsigned)category >= 32u) return 0;
    return (enabled_categories & (1u << (unsigned)category)) != 0;
}

void log_message(LogCategory category, const char *file, int line,
                 const char *format, ...)
{
    va_list args;
    if ((unsigned)category >= LOG_COUNT || !log_is_enabled(category)) return;

    printf("[%s] %s:%d: ", log_category_names[category], file, line);
    va_start(args, format);
    vprintf(format, args);
    va_end(args);
    putchar('\n');
    fflush(stdout);
}
