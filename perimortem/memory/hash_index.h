// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#ifndef PERIMORTEM_MEMORY_HASH_INDEX_H
#define PERIMORTEM_MEMORY_HASH_INDEX_H

#include "perimortem/memory/aligned_buffer.h"

// Hash Index stores only precomputed hashes and typed entry indices. The owner
// keeps the actual keys and values, compares its own key type, and decides how
// entries move. This makes collision handling reusable without creating an
// erased generic Map or a second authority over collection contents.
//
// Open addressing keeps compiler maps compact and cache friendly. Lookup may
// return several candidates with the same hash, so callers always compare the
// real key before accepting a match. Removal repairs the probe cluster and
// therefore introduces no tombstone state that could accumulate across a long
// editing session.
struct perimortem_hash_index {
  struct perimortem_aligned_buffer storage;
  perimortem_count bucket_count;
  perimortem_count size;
};

struct perimortem_hash_index_match {
  perimortem_count bucket;
  perimortem_count entry;
};

PERIMORTEM_EXTERN_C void perimortem_hash_index_release(
    struct perimortem_hash_index* index);
PERIMORTEM_EXTERN_C void perimortem_hash_index_clear(
    struct perimortem_hash_index* index);
PERIMORTEM_EXTERN_C perimortem_bool perimortem_hash_index_reserve(
    struct perimortem_hash_index* index,
    perimortem_count required_entries);
PERIMORTEM_EXTERN_C perimortem_bool perimortem_hash_index_insert(
    struct perimortem_hash_index* index,
    uint64_t hash,
    perimortem_count entry);
PERIMORTEM_EXTERN_C perimortem_bool perimortem_hash_index_remove(
    struct perimortem_hash_index* index,
    uint64_t hash,
    perimortem_count entry);
PERIMORTEM_EXTERN_C perimortem_bool perimortem_hash_index_replace(
    struct perimortem_hash_index* index,
    uint64_t hash,
    perimortem_count old_entry,
    perimortem_count new_entry);
PERIMORTEM_EXTERN_C perimortem_bool perimortem_hash_index_find_first(
    const struct perimortem_hash_index* index,
    uint64_t hash,
    struct perimortem_hash_index_match* match);
PERIMORTEM_EXTERN_C perimortem_bool perimortem_hash_index_find_next(
    const struct perimortem_hash_index* index,
    uint64_t hash,
    struct perimortem_hash_index_match* match);

#endif
