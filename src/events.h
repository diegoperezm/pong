#ifndef EVENTS_H
#define EVENTS_H

#include <stdint.h>
#include <stdbool.h>
#include "game_types.h"

#define EVENT_RING_CAPACITY 256u

typedef enum {
    EVT_NONE = 0,
    EVT_MATCH_STARTED,
    EVT_TILE_SLID,
    EVT_MATCH_ENDED
} EventType;

typedef struct {
    uint32_t tick;
    EventType type;
    int tile_value;
    int score_player;
} SimEvent;

typedef struct {
    SimEvent events[EVENT_RING_CAPACITY];
    uint64_t head;
} EventQueue;

typedef struct {
    uint64_t next;
    uint32_t lost;
} EventCursor;

void event_queue_clear(EventQueue *q);
void event_queue_push(EventQueue *q, const SimEvent *e);
uint64_t event_queue_total(const EventQueue *q);
bool event_queue_recent(const EventQueue *q, uint32_t back, SimEvent *out);
void event_cursor_sync(const EventQueue *q, EventCursor *c);
bool event_cursor_read(const EventQueue *q, EventCursor *c, SimEvent *out);
const char *event_type_name(EventType type);

#endif
