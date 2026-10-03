#ifndef EVENTS_H
#define EVENTS_H

/* Outbound game events: simulation -> everything else.
 *
 * Plain data only. An event never carries an entity handle, because the
 * ball it describes may already be destroyed by the time anyone reads it.
 * Identity, when needed, is a per-match "ball serial" that is never reused.
 *
 * Ordering rule (relied on by replays and checksums):
 *   by tick, then by the fixed order of simulation systems, then by ball
 *   serial (ascending). */

#include <stdbool.h>
#include <stdint.h>
#include "game_types.h"

#define EVENT_RING_CAPACITY 256u   /* power of two */

_Static_assert((EVENT_RING_CAPACITY & (EVENT_RING_CAPACITY - 1u)) == 0u,
               "EVENT_RING_CAPACITY must be a power of two");

typedef enum {
    EVT_NONE = 0,
    EVT_MATCH_STARTED,      /* tick 0                                          */
    EVT_BALL_SERVED,        /* ball_serial, x/y, vx/vy                         */
    EVT_BALL_HIT_PADDLE,    /* ball_serial, x/y, side = which paddle           */
    EVT_BALL_HIT_WALL,      /* ball_serial, x/y, wall = top or bottom          */
    EVT_POINT_SCORED,       /* ball_serial, x/y where it left, side = scorer   */
    EVT_MATCH_ENDED         /* side = winner                                   */
} EventType;

typedef struct {
    uint32_t  tick;
    EventType type;
    uint32_t  ball_serial;
    float     x, y;           /* px, ball centre */
    float     vx, vy;         /* px/s */
    Side      side;
    Wall      wall;
    int       score_player;   /* scores AFTER this event */
    int       score_enemy;
} SimEvent;

/* Bounded history ring. Oldest entries are overwritten. `head` counts every
 * event pushed since the last clear, so (head - 1) is the newest. */
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
