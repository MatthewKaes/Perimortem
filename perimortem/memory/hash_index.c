// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#include "perimortem/memory/hash_index.h"

struct perimortem_hash_bucket {
  uint64_t hash;
  perimortem_count entry;
};

static const perimortem_count empty_entry = UINT64_MAX;

static struct perimortem_hash_bucket* writable_buckets(
    struct perimortem_hash_index* index) {
  return (struct perimortem_hash_bucket*)index->storage.data;
}

static const struct perimortem_hash_bucket* readable_buckets(
    const struct perimortem_hash_index* index) {
  return (const struct perimortem_hash_bucket*)index->storage.data;
}

static perimortem_count home(
    const struct perimortem_hash_index* index,
    uint64_t hash) {
  return hash & (index->bucket_count - 1);
}

static perimortem_count distance(
    const struct perimortem_hash_index* index,
    perimortem_count from,
    perimortem_count to) {
  return (to - from) & (index->bucket_count - 1);
}

static void initialize_buckets(
    struct perimortem_hash_bucket* values,
    perimortem_count count) {
  for (perimortem_count position = 0; position < count; ++position) {
    values[position].hash = 0;
    values[position].entry = empty_entry;
  }
}

static perimortem_bool insert_unchecked(
    struct perimortem_hash_index* index,
    uint64_t hash,
    perimortem_count entry) {
  struct perimortem_hash_bucket* values = writable_buckets(index);
  perimortem_count position = home(index, hash);
  while (values[position].entry != empty_entry) {
    if (values[position].hash == hash && values[position].entry == entry) {
      return PERIMORTEM_FALSE;
    }

    position = (position + 1) & (index->bucket_count - 1);
  }

  values[position].hash = hash;
  values[position].entry = entry;
  ++index->size;
  return PERIMORTEM_TRUE;
}

static perimortem_bool grow(
    struct perimortem_hash_index* index,
    perimortem_count bucket_count) {
  if (bucket_count > UINT64_MAX / sizeof(struct perimortem_hash_bucket)) {
    return PERIMORTEM_FALSE;
  }

  struct perimortem_aligned_buffer replacement = {0};
  if (!perimortem_aligned_buffer_create(
          bucket_count * sizeof(struct perimortem_hash_bucket),
          _Alignof(struct perimortem_hash_bucket), &replacement)) {
    return PERIMORTEM_FALSE;
  }

  struct perimortem_hash_index rebuilt = {
    .storage = replacement,
    .bucket_count = bucket_count,
    .size = 0,
  };

  initialize_buckets(writable_buckets(&rebuilt), bucket_count);

  const struct perimortem_hash_bucket* previous = readable_buckets(index);
  for (perimortem_count position = 0; position < index->bucket_count;
       ++position) {
    if (previous[position].entry != empty_entry &&
        !insert_unchecked(
            &rebuilt, previous[position].hash, previous[position].entry)) {
      perimortem_aligned_buffer_release(&rebuilt.storage);
      return PERIMORTEM_FALSE;
    }
  }

  perimortem_aligned_buffer_release(&index->storage);
  *index = rebuilt;
  return PERIMORTEM_TRUE;
}

void perimortem_hash_index_release(struct perimortem_hash_index* index) {
  if (index == 0) {
    return;
  }

  perimortem_aligned_buffer_release(&index->storage);
  index->bucket_count = 0;
  index->size = 0;
}

void perimortem_hash_index_clear(struct perimortem_hash_index* index) {
  if (index == 0 || index->bucket_count == 0) {
    return;
  }

  initialize_buckets(writable_buckets(index), index->bucket_count);
  index->size = 0;
}

perimortem_bool perimortem_hash_index_reserve(
    struct perimortem_hash_index* index,
    perimortem_count required_entries) {
  if (index == 0 || required_entries > UINT64_MAX / 10) {
    return PERIMORTEM_FALSE;
  }

  const perimortem_count maximum_buckets =
      PERIMORTEM_BIBLIOTHECA_MAXIMUM_CAPACITY /
      sizeof(struct perimortem_hash_bucket);
  if (required_entries > maximum_buckets * 7 / 10) {
    return PERIMORTEM_FALSE;
  }

  perimortem_count required_buckets = 2;
  while (required_entries * 10 > required_buckets * 7) {
    if (required_buckets > UINT64_MAX / 2) {
      return PERIMORTEM_FALSE;
    }

    required_buckets *= 2;
  }

  return required_buckets <= index->bucket_count
             ? PERIMORTEM_TRUE
             : grow(index, required_buckets);
}

perimortem_bool perimortem_hash_index_insert(
    struct perimortem_hash_index* index,
    uint64_t hash,
    perimortem_count entry) {
  if (index == 0 || entry == empty_entry ||
      !perimortem_hash_index_reserve(index, index->size + 1)) {
    return PERIMORTEM_FALSE;
  }

  return insert_unchecked(index, hash, entry);
}

perimortem_bool perimortem_hash_index_find_first(
    const struct perimortem_hash_index* index,
    uint64_t hash,
    struct perimortem_hash_index_match* match) {
  if (index == 0 || match == 0 || index->bucket_count == 0) {
    return PERIMORTEM_FALSE;
  }

  const struct perimortem_hash_bucket* values = readable_buckets(index);
  perimortem_count position = home(index, hash);
  while (values[position].entry != empty_entry) {
    if (values[position].hash == hash) {
      match->bucket = position;
      match->entry = values[position].entry;
      return PERIMORTEM_TRUE;
    }

    position = (position + 1) & (index->bucket_count - 1);
  }

  return PERIMORTEM_FALSE;
}

perimortem_bool perimortem_hash_index_find_next(
    const struct perimortem_hash_index* index,
    uint64_t hash,
    struct perimortem_hash_index_match* match) {
  if (index == 0 || match == 0 || match->bucket >= index->bucket_count) {
    return PERIMORTEM_FALSE;
  }

  const struct perimortem_hash_bucket* values = readable_buckets(index);
  perimortem_count position = (match->bucket + 1) & (index->bucket_count - 1);
  while (values[position].entry != empty_entry) {
    if (values[position].hash == hash) {
      match->bucket = position;
      match->entry = values[position].entry;
      return PERIMORTEM_TRUE;
    }

    position = (position + 1) & (index->bucket_count - 1);
  }

  return PERIMORTEM_FALSE;
}

perimortem_bool perimortem_hash_index_remove(
    struct perimortem_hash_index* index,
    uint64_t hash,
    perimortem_count entry) {
  if (index == 0 || index->bucket_count == 0) {
    return PERIMORTEM_FALSE;
  }

  struct perimortem_hash_bucket* values = writable_buckets(index);
  perimortem_count hole = home(index, hash);
  while (values[hole].entry != empty_entry &&
         (values[hole].hash != hash || values[hole].entry != entry)) {
    hole = (hole + 1) & (index->bucket_count - 1);
  }

  if (values[hole].entry == empty_entry) {
    return PERIMORTEM_FALSE;
  }

  perimortem_count next = (hole + 1) & (index->bucket_count - 1);
  while (values[next].entry != empty_entry) {
    const perimortem_count next_home = home(index, values[next].hash);
    if (distance(index, next_home, hole) < distance(index, next_home, next)) {
      values[hole] = values[next];
      hole = next;
    }

    next = (next + 1) & (index->bucket_count - 1);
  }

  values[hole].hash = 0;
  values[hole].entry = empty_entry;
  --index->size;
  return PERIMORTEM_TRUE;
}

perimortem_bool perimortem_hash_index_replace(
    struct perimortem_hash_index* index,
    uint64_t hash,
    perimortem_count old_entry,
    perimortem_count new_entry) {
  struct perimortem_hash_index_match match;
  if (new_entry == empty_entry ||
      !perimortem_hash_index_find_first(index, hash, &match)) {
    return PERIMORTEM_FALSE;
  }

  do {
    if (match.entry == old_entry) {
      writable_buckets(index)[match.bucket].entry = new_entry;
      return PERIMORTEM_TRUE;
    }

  } while (perimortem_hash_index_find_next(index, hash, &match));

  return PERIMORTEM_FALSE;
}
