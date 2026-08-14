#include "log.h"

#include <stdarg.h>
#include <stdio.h>


/* ============================================================
 * STATE
 * ============================================================
 */

static int
log_enabled[LOG_COUNT];


/* ============================================================
 * CATEGORY NAMES
 * ============================================================
 */

static const char *
log_category_name(
    LogCategory category
)
{
    switch (category) {

        case LOG_GAME:
            return "GAME";

        case LOG_INPUT:
            return "INPUT";

        case LOG_SIMULATION:
            return "SIMULATION";

        case LOG_COLLISION:
            return "COLLISION";

        case LOG_ENTITY:
            return "ENTITY";

        case LOG_AI:
            return "AI";

        case LOG_RENDER:
            return "RENDER";

        case PONG_LOG_DEBUG:
            return "DEBUG";

        default:
            return "UNKNOWN";
    }
}


/* ============================================================
 * INITIALIZATION
 * ============================================================
 */

void
log_init(void)
{
    /*
     * Disable everything by default.
     */

    for (int i = 0; i < LOG_COUNT; ++i) {
        log_enabled[i] = 0;
    }


    /*
     * Enable the categories that are
     * useful during normal development.
     */

    log_enabled[LOG_GAME] = 1;
    log_enabled[LOG_COLLISION] = 1;
    log_enabled[LOG_ENTITY] = 1;
    log_enabled[LOG_AI] = 1;
}


/* ============================================================
 * ENABLE/DISABLE
 * ============================================================
 */

void
log_set_enabled(
    LogCategory category,
    int enabled
)
{
    if (category < 0 || category >= LOG_COUNT) {
        return;
    }

    log_enabled[category] = enabled != 0; 
}


/* ============================================================
 * QUERY
 * ============================================================
 */

int
log_is_enabled(
    LogCategory category
)
{
    if (category < 0 || category >= LOG_COUNT) {
        return 0;
    }

    return log_enabled[category];
}


/* ============================================================
 * LOG MESSAGE
 * ============================================================
 */

void
log_message(
    LogCategory category,
    const char *file,
    int line,
    const char *format,
    ...
)
{
    if (!log_is_enabled(category))
        return;


    printf(
        "[%s] %s:%d: ",
        log_category_name(category),
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

