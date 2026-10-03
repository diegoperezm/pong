#include "replay.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char REPLAY_MAGIC[8] = { 'P', 'O', 'N', 'G', 'R', 'P', 'L', '1' };

/* Sanity cap used when loading untrusted files. */
#define REPLAY_MAX_RECORDS (1u << 24)

/* ============================================================
 * LIFECYCLE
 * ============================================================ */

void replay_init(ReplayData *r) {
    memset(r, 0, sizeof *r);
}

void replay_free(ReplayData *r) {
    free(r->changes);
    free(r->checkpoints);
    memset(r, 0, sizeof *r);
}

void replay_begin(ReplayData *r, uint64_t seed, uint64_t config_hash) {
    /* keep the allocations, reset everything else */
    r->format_version   = REPLAY_FORMAT_VERSION;
    r->sim_hz           = (uint32_t)SIM_HZ;
    r->config_hash      = config_hash;
    r->seed             = seed;
    r->tick_count       = 0;
    r->final_checksum   = 0;
    r->complete         = false;
    r->change_count     = 0;
    r->checkpoint_count = 0;
    r->last_recorded    = (TickInput){ false, false };
    r->failed           = false;
}

/* ============================================================
 * RECORDING
 * ============================================================ */

static bool reserve_changes(ReplayData *r) {
    if (r->change_count < r->change_cap) return true;
    uint32_t cap = r->change_cap ? r->change_cap * 2u : 256u;
    ReplayChange *p = realloc(r->changes, (size_t)cap * sizeof *p);
    if (!p) return false;
    r->changes = p;
    r->change_cap = cap;
    return true;
}

static bool reserve_checkpoints(ReplayData *r) {
    if (r->checkpoint_count < r->checkpoint_cap) return true;
    uint32_t cap = r->checkpoint_cap ? r->checkpoint_cap * 2u : 64u;
    ReplayCheckpoint *p = realloc(r->checkpoints, (size_t)cap * sizeof *p);
    if (!p) return false;
    r->checkpoints = p;
    r->checkpoint_cap = cap;
    return true;
}

void replay_record_tick(ReplayData *r, uint32_t tick, TickInput in) {
    if (r->failed) return;
    if (tick_input_equal(in, r->last_recorded)) return;
    if (!reserve_changes(r)) { r->failed = true; return; }

    r->changes[r->change_count++] = (ReplayChange){
        .tick = tick,
        .up   = in.up   ? 1u : 0u,
        .down = in.down ? 1u : 0u
    };
    r->last_recorded = in;
}

void replay_record_checkpoint(ReplayData *r, uint32_t tick, uint64_t checksum) {
    if (r->failed) return;
    if (!reserve_checkpoints(r)) { r->failed = true; return; }

    r->checkpoints[r->checkpoint_count++] = (ReplayCheckpoint){ tick, checksum };
}

void replay_finish(ReplayData *r, uint32_t tick_count, uint64_t final_checksum) {
    r->tick_count     = tick_count;
    r->final_checksum = final_checksum;
    r->complete       = !r->failed;
}

bool replay_checkpoint_for(const ReplayData *r, uint32_t tick, uint64_t *checksum) {
    uint32_t lo = 0, hi = r->checkpoint_count;
    while (lo < hi) {
        uint32_t mid = lo + (hi - lo) / 2u;
        uint32_t t = r->checkpoints[mid].tick;
        if (t == tick) { *checksum = r->checkpoints[mid].checksum; return true; }
        if (t < tick) lo = mid + 1u; else hi = mid;
    }
    return false;
}

/* ============================================================
 * PLAYBACK
 * ============================================================ */

void replay_cursor_reset(ReplayCursor *c) {
    c->next_change = 0;
    c->held = (TickInput){ false, false };
}

TickInput replay_input_at(const ReplayData *r, ReplayCursor *c, uint32_t tick) {
    while (c->next_change < r->change_count &&
           r->changes[c->next_change].tick <= tick) {
        c->held.up   = r->changes[c->next_change].up   != 0;
        c->held.down = r->changes[c->next_change].down != 0;
        c->next_change++;
    }
    return c->held;
}

/* ============================================================
 * FILE I/O (little-endian, explicit field order)
 * ============================================================ */

static bool put_u8(FILE *f, uint8_t v) { return fputc(v, f) != EOF; }

static bool put_u32(FILE *f, uint32_t v) {
    uint8_t b[4] = { (uint8_t)v, (uint8_t)(v >> 8), (uint8_t)(v >> 16), (uint8_t)(v >> 24) };
    return fwrite(b, 1, 4, f) == 4;
}

static bool put_u64(FILE *f, uint64_t v) {
    return put_u32(f, (uint32_t)v) && put_u32(f, (uint32_t)(v >> 32));
}

static bool get_u8(FILE *f, uint8_t *v) {
    int c = fgetc(f);
    if (c == EOF) return false;
    *v = (uint8_t)c;
    return true;
}

static bool get_u32(FILE *f, uint32_t *v) {
    uint8_t b[4];
    if (fread(b, 1, 4, f) != 4) return false;
    *v = (uint32_t)b[0] | ((uint32_t)b[1] << 8) | ((uint32_t)b[2] << 16) | ((uint32_t)b[3] << 24);
    return true;
}

static bool get_u64(FILE *f, uint64_t *v) {
    uint32_t lo, hi;
    if (!get_u32(f, &lo) || !get_u32(f, &hi)) return false;
    *v = (uint64_t)lo | ((uint64_t)hi << 32);
    return true;
}

bool replay_save(const ReplayData *r, const char *path) {
    if (!r->complete || r->failed) return false;

    FILE *f = fopen(path, "wb");
    if (!f) return false;

    bool ok = fwrite(REPLAY_MAGIC, 1, sizeof REPLAY_MAGIC, f) == sizeof REPLAY_MAGIC
           && put_u32(f, r->format_version)
           && put_u32(f, r->sim_hz)
           && put_u64(f, r->config_hash)
           && put_u64(f, r->seed)
           && put_u32(f, r->tick_count)
           && put_u64(f, r->final_checksum)
           && put_u32(f, r->change_count)
           && put_u32(f, r->checkpoint_count);

    for (uint32_t i = 0; ok && i < r->change_count; ++i) {
        ok = put_u32(f, r->changes[i].tick)
          && put_u8(f, r->changes[i].up)
          && put_u8(f, r->changes[i].down);
    }
    for (uint32_t i = 0; ok && i < r->checkpoint_count; ++i) {
        ok = put_u32(f, r->checkpoints[i].tick)
          && put_u64(f, r->checkpoints[i].checksum);
    }

    if (fclose(f) != 0) ok = false;
    return ok;
}

bool replay_load(ReplayData *out, const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f) return false;

    ReplayData tmp;
    replay_init(&tmp);

    char magic[8];
    bool ok = fread(magic, 1, sizeof magic, f) == sizeof magic
           && memcmp(magic, REPLAY_MAGIC, sizeof magic) == 0
           && get_u32(f, &tmp.format_version)
           && get_u32(f, &tmp.sim_hz)
           && get_u64(f, &tmp.config_hash)
           && get_u64(f, &tmp.seed)
           && get_u32(f, &tmp.tick_count)
           && get_u64(f, &tmp.final_checksum)
           && get_u32(f, &tmp.change_count)
           && get_u32(f, &tmp.checkpoint_count);

    ok = ok && tmp.format_version == REPLAY_FORMAT_VERSION
            && tmp.sim_hz > 0
            && tmp.change_count     <= REPLAY_MAX_RECORDS
            && tmp.checkpoint_count <= REPLAY_MAX_RECORDS;

    if (ok && tmp.change_count > 0) {
        tmp.changes = malloc((size_t)tmp.change_count * sizeof *tmp.changes);
        tmp.change_cap = tmp.change_count;
        ok = tmp.changes != NULL;
    }
    if (ok && tmp.checkpoint_count > 0) {
        tmp.checkpoints = malloc((size_t)tmp.checkpoint_count * sizeof *tmp.checkpoints);
        tmp.checkpoint_cap = tmp.checkpoint_count;
        ok = tmp.checkpoints != NULL;
    }

    for (uint32_t i = 0; ok && i < tmp.change_count; ++i) {
        ReplayChange *c = &tmp.changes[i];
        ok = get_u32(f, &c->tick) && get_u8(f, &c->up) && get_u8(f, &c->down)
          && c->up <= 1 && c->down <= 1
          && c->tick < tmp.tick_count
          && (i == 0 || c->tick > tmp.changes[i - 1].tick);   /* strictly ascending */
    }
    for (uint32_t i = 0; ok && i < tmp.checkpoint_count; ++i) {
        ReplayCheckpoint *c = &tmp.checkpoints[i];
        ok = get_u32(f, &c->tick) && get_u64(f, &c->checksum)
          && c->tick <= tmp.tick_count
          && (i == 0 || c->tick > tmp.checkpoints[i - 1].tick);
    }

    fclose(f);

    if (!ok) {
        replay_free(&tmp);
        return false;
    }

    tmp.complete = true;
    replay_free(out);
    *out = tmp;
    return true;
}
