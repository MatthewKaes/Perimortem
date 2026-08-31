// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef PERIMORTEM_CORE_HASH_H
#define PERIMORTEM_CORE_HASH_H

#include "perimortem/core/perimortem.h"

// Hash is the stable noncryptographic digest used by compiler collections and
// persisted fingerprints. Its fixed constants keep Package observations
// repeatable across processes, unlike a salted general purpose hash table.
// That stability also means it offers no collision defense for adversarial
// input and should not be used as a security boundary.
//
// The byte path is tuned for the short names and routes common in TTX graphs.
// It accepts unaligned storage and reads only within the supplied view. Numeric
// hashing uses the same avalanche so typed owners can feed one Hash Index
// without host template dispatch.
PERIMORTEM_EXTERN_C uint64_t
    perimortem_hash_bytes(struct perimortem_bytes bytes);
PERIMORTEM_EXTERN_C uint64_t perimortem_hash_u64(uint64_t value);
PERIMORTEM_EXTERN_C uint64_t
    perimortem_hash_combine(uint64_t seed, uint64_t value);

#endif
