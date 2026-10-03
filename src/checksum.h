#ifndef CHECKSUM_H
#define CHECKSUM_H

#include <stdint.h>
#include "sim_types.h"

/* Hash of everything that defines the simulation's future: tick, scores,
 * match flags, RNG state, paddles, powerups, and every ball (walked in
 * SERIAL order, never container order). Equal checksums on two runs mean
 * the runs have not diverged. Events and particles are deliberately NOT
 * included: they are outputs, not state. */
uint64_t sim_checksum(const SimulationState *state);

#endif
