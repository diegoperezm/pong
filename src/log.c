#include "log.h"

#include <stdarg.h>
#include <stdio.h>


/* ============================================================
 * STATE
 * ============================================================
 */

static uint32_t enabled_categories;

#define LOG_CATEGORY_NAME(category, name) [category] = name,

const char* log_category_name[LOG_COUNT] = {
    LOG_CATEGORIES(LOG_CATEGORY_NAME)
};

#undef LOG_CATEGORY_NAME

void log_init(void)
{
 enabled_categories = 0;
 
}

void log_enable(LogCategory category)
{
  enabled_categories |= (1u << category);
}

void log_disable(LogCategory category)
{
  enabled_categories &= ~(1u << category);
}

int log_is_enabled(LogCategory category)
{
  return  (enabled_categories & (1u << category)) != 0; 
}


void
log_message(
    LogCategory category,
    const char* file,
    int         line,
    const char* format,
    ...
)
{
    if (!log_is_enabled(category))
        return;


    printf(
        "[%s] %s:%d: ",
        log_category_name[category],
        file,
        line
    );


    va_list args;

    va_start(args, format);

    vprintf(
        format,
        args
    );

    va_end(args);


    printf("\n");

    fflush(stdout);
}

