#ifndef EVENTS_H
#define EVENTS_H

#include <stdbool.h>
#include <stdint.h>

#define EVENT_RING_CAPACITY 256u

#if (EVENT_RING_CAPACITY & (EVENT_RING_CAPACITY - 1u)) != 0
#error "EVENT_RING_CAPACITY must be a power of two"
#endif

typedef enum {
    EVT_NONE = 0,
    EVT_MATCH_STARTED,
    EVT_TILE_SLID,
    EVT_MATCH_ENDED
} EventType;

typedef struct {
    uint32_t move_count;
    EventType type;
    uint8_t tile_value;
    uint8_t from_index;
    uint8_t to_index;
} SimEvent;

typedef struct {
    SimEvent events[EVENT_RING_CAPACITY];
    uint64_t head;
} EventQueue;

typedef struct {
    uint64_t next;
    uint32_t lost;
} EventCursor;

void event_queue_clear(EventQueue *queue);
void event_queue_push(EventQueue *queue, const SimEvent *event);
uint64_t event_queue_total(const EventQueue *queue);
bool event_queue_recent(const EventQueue *queue, uint32_t back, SimEvent *out);
void event_cursor_sync(const EventQueue *queue, EventCursor *cursor);
bool event_cursor_read(const EventQueue *queue, EventCursor *cursor, SimEvent *out);
const char *event_type_name(EventType type);

#endif
