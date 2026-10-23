#ifndef EVENTS_H
#define EVENTS_H

#include <stdbool.h>
#include <stdint.h>
#include "sim_types.h"

#define EVENT_RING_CAPACITY 256u   /* power of two */

_Static_assert((EVENT_RING_CAPACITY & (EVENT_RING_CAPACITY - 1u)) == 0u,
               "EVENT_RING_CAPACITY must be a power of two");

typedef enum {
    EVT_NONE = 0,
    EVT_MATCH_STARTED,      /* tick 0 */
    EVT_CUSTOM_EVENT_1,     /* Generic placeholder for game events */
    EVT_CUSTOM_EVENT_2,     /* Generic placeholder for game events */
    EVT_MATCH_ENDED         /* match / session over */
} EventType;

typedef struct {
    uint32_t  tick;
    EventType type;
    uint32_t  entity_serial;
    float     x, y;           /* coordinates / position */
    float     vx, vy;         /* velocities */
    int       param1;         /* generic custom parameters */
    int       param2;
} SimEvent;

/* Bounded history ring. Oldest entries are overwritten. */
typedef struct {
    SimEvent events[EVENT_RING_CAPACITY];
    uint64_t head;
} EventQueue;

/* Each consumer owns a cursor, so consumers never interfere. */
typedef struct {
    uint64_t next;   /* sequence number of the next event to read */
    uint32_t lost;   /* events overwritten before this consumer read them */
} EventCursor;

void     event_queue_clear(EventQueue *q);
void     event_queue_push(EventQueue *q, const SimEvent *e);   /* q may be NULL */
uint64_t event_queue_total(const EventQueue *q);

/* back = 0 is the newest event. Returns false if there is no such event. */
bool     event_queue_recent(const EventQueue *q, uint32_t back, SimEvent *out);

void     event_cursor_sync(const EventQueue *q, EventCursor *c);   /* skip to head */
bool     event_cursor_read(const EventQueue *q, EventCursor *c, SimEvent *out);

const char *event_type_name(EventType type);

#endif


