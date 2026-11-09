#include "events.h"

#define RING_MASK (EVENT_RING_CAPACITY - 1u)

void event_queue_clear(EventQueue *queue)
{
    if (queue) queue->head = 0;
}

void event_queue_push(EventQueue *queue, const SimEvent *event)
{
    if (!queue || !event) return;
    queue->events[queue->head & RING_MASK] = *event;
    queue->head++;
}

uint64_t event_queue_total(const EventQueue *queue)
{
    return queue ? queue->head : 0;
}

bool event_queue_recent(const EventQueue *queue, uint32_t back, SimEvent *out)
{
    uint64_t available;
    if (!queue || !out) return false;
    available = queue->head < EVENT_RING_CAPACITY ? queue->head : EVENT_RING_CAPACITY;
    if ((uint64_t)back >= available) return false;
    *out = queue->events[(queue->head - 1u - back) & RING_MASK];
    return true;
}

void event_cursor_sync(const EventQueue *queue, EventCursor *cursor)
{
    if (!cursor) return;
    cursor->next = queue ? queue->head : 0;
    cursor->lost = 0;
}

bool event_cursor_read(const EventQueue *queue, EventCursor *cursor, SimEvent *out)
{
    uint64_t oldest;
    if (!queue || !cursor || !out) return false;
    if (cursor->next > queue->head) cursor->next = queue->head;
    oldest = queue->head > EVENT_RING_CAPACITY ? queue->head - EVENT_RING_CAPACITY : 0;
    if (cursor->next < oldest) {
        cursor->lost += (uint32_t)(oldest - cursor->next);
        cursor->next = oldest;
    }
    if (cursor->next >= queue->head) return false;
    *out = queue->events[cursor->next & RING_MASK];
    cursor->next++;
    return true;
}

const char *event_type_name(EventType type)
{
    switch (type) {
        case EVT_MATCH_STARTED: return "MATCH_START";
        case EVT_TILE_SLID:     return "TILE_SLID";
        case EVT_MATCH_ENDED:   return "MATCH_END";
        default:                return "NONE";
    }
}
