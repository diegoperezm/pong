#include "events.h"

#define RING_MASK (EVENT_RING_CAPACITY - 1u)

void event_queue_clear(EventQueue *q) {
    if (q) q->head = 0;
}

void event_queue_push(EventQueue *q, const SimEvent *e) {
    if (!q || !e) return;
    q->events[q->head & RING_MASK] = *e;
    q->head++;
}

uint64_t event_queue_total(const EventQueue *q) {
    return q ? q->head : 0;
}

bool event_queue_recent(const EventQueue *q, uint32_t back, SimEvent *out) {
    if (!q || !out) return false;
    uint64_t available = q->head < EVENT_RING_CAPACITY ? q->head : EVENT_RING_CAPACITY;
    if ((uint64_t)back >= available) return false;
    *out = q->events[(q->head - 1u - back) & RING_MASK];
    return true;
}

void event_cursor_sync(const EventQueue *q, EventCursor *c) {
    c->next = q ? q->head : 0;
}

bool event_cursor_read(const EventQueue *q, EventCursor *c, SimEvent *out) {
    if (!q || !c || !out) return false;

    if (c->next > q->head)            /* queue was cleared under us */
        c->next = q->head;

    uint64_t oldest = q->head > EVENT_RING_CAPACITY ? q->head - EVENT_RING_CAPACITY : 0;
    if (c->next < oldest) {           /* fell behind: those events are gone */
        c->lost += (uint32_t)(oldest - c->next);
        c->next = oldest;
    }

    if (c->next >= q->head) return false;

    *out = q->events[c->next & RING_MASK];
    c->next++;
    return true;
}

const char *event_type_name(EventType type) {
    switch (type) {
        case EVT_MATCH_STARTED:   return "MATCH_START";
        case EVT_BALL_SERVED:     return "SERVE";
        case EVT_BALL_HIT_PADDLE: return "PADDLE_HIT";
        case EVT_BALL_HIT_WALL:   return "WALL_HIT";
        case EVT_POINT_SCORED:    return "SCORE";
        case EVT_MATCH_ENDED:     return "MATCH_END";
        default:                  return "NONE";
    }
}
