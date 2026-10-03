#ifndef REPLAY_H
#define REPLAY_H

/* The input queue, as data.
 *
 * A match is fully described by: seed + per-tick input. This module stores
 * that stream sparsely (only the ticks where input CHANGED), plus periodic
 * state checksums so a replay can prove it has not diverged.
 *
 * "Held" semantics: before the first change the input is {up=0, down=0};
 * a change at tick T applies to tick T and every later tick until the next
 * change. */

#include <stdbool.h>
#include <stdint.h>
#include "game_types.h"

#define REPLAY_FORMAT_VERSION       1u
#define REPLAY_CHECKPOINT_INTERVAL  120u   /* ticks (1 s at 120 Hz) */

typedef enum {
    REPLAY_RESULT_NONE = 0,
    REPLAY_RESULT_OK,
    REPLAY_RESULT_DESYNC
} ReplayResult;

typedef struct {
    uint32_t tick;
    uint8_t  up;
    uint8_t  down;
} ReplayChange;

typedef struct {
    uint32_t tick;        /* checksum of sim state BEFORE this tick runs */
    uint64_t checksum;
} ReplayCheckpoint;

typedef struct {
    /* header */
    uint32_t format_version;
    uint32_t sim_hz;
    uint64_t config_hash;
    uint64_t seed;
    uint32_t tick_count;        /* ticks executed by the recorded match */
    uint64_t final_checksum;    /* state after the last executed tick   */
    bool     complete;          /* finished and safe to save            */

    /* body */
    ReplayChange     *changes;
    uint32_t          change_count, change_cap;
    ReplayCheckpoint *checkpoints;
    uint32_t          checkpoint_count, checkpoint_cap;

    /* recorder state */
    TickInput last_recorded;
    bool      failed;           /* allocation failed while recording */
} ReplayData;

void replay_init(ReplayData *r);                 /* zero, no allocation */
void replay_free(ReplayData *r);
void replay_begin(ReplayData *r, uint64_t seed, uint64_t config_hash);

/* Recording (call once per executed tick, in order). */
void replay_record_tick(ReplayData *r, uint32_t tick, TickInput in);
void replay_record_checkpoint(ReplayData *r, uint32_t tick, uint64_t checksum);
void replay_finish(ReplayData *r, uint32_t tick_count, uint64_t final_checksum);

bool replay_checkpoint_for(const ReplayData *r, uint32_t tick, uint64_t *checksum);

/* Persistence. Fixed little-endian binary layout; load validates everything
 * and returns false (leaving `r` untouched) on any problem. */
bool replay_save(const ReplayData *r, const char *path);
bool replay_load(ReplayData *r, const char *path);

/* Playback. Ticks must be requested in increasing order; reset on seek. */
typedef struct {
    uint32_t  next_change;
    TickInput held;
} ReplayCursor;

void      replay_cursor_reset(ReplayCursor *c);
TickInput replay_input_at(const ReplayData *r, ReplayCursor *c, uint32_t tick);

#endif
