// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include <x86intrin.h>

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/hash.hpp"

#include "perimortem/utility/pair.hpp"

namespace Perimortem::Memory::Dynamic {

enum class MapVectorization { Full, Partial, Scalar };

// Unordered flat map with a two stage look up to optimize for insert, delete
// and find operations. Optimized for value types.
//
// The table is optimized for the AMD Zen architecture and will most likely
// perform slightly worse than absl::flat_hash_map and boost::unordered_flat_map
// on Intel processors (I haven't tested but based on Anger's data up to
// TigerLake the latency points to boost's favor).
//
// The table is also able to optimize its throughput on text keys by supporting
// Perimortem's hash function which is optimized for text keys to shave off
// cycles compared to City or Murmur3, losing performance mostly on sparse bit
// hashes.
//
// Since Perimortem uses 32 byte wide AVX2 registers the buckets are filled up
// linearly both for improved instruction throughput (and very minor branch
// performance for iteration) and to improve cache performance for small key /
// value pairs, although any major gains are offset by storing the 8 hash
// inline.
//
// Map types:
// * Scalar is optimized for speed and size and should be used for any table
//   that uses View::Bytes for keys.
//
// * Partial is optimized for speed on medium sized tables with integer key
//   mappings.
//
// * Full should only be used on tables with 1000+ integer keys.
//
// Full and Partial typically outperform Scalar when your lookup hit rate ~<25%.
template <
    typename key_type,
    typename value_type,
    MapVectorization vector_mode = MapVectorization::Scalar>
class Map {
 public:
  using Entry = Utility::Pair<key_type, value_type>;

  // Internal configuration of data type
 private:
  struct SlotInlineHash {
    Entry entry;
    Bits_64 hash;
  };

  struct SlotEntryOnly {
    Entry entry;
  };

  template <MapVectorization>
  struct VectorType {
    using Type = __m256i;
    using MaskType = Bits_32;
    using KeyType = Bits_8;
    using InputHash = Bits_64;
    using SlotType = SlotInlineHash;
  };

  template <>
  struct VectorType<MapVectorization::Partial> {
    using Type = __m128i;
    using MaskType = Bits_16;
    using KeyType = Bits_8;
    using InputHash = Bits_64;
    using SlotType = SlotInlineHash;
  };

  template <>
  struct VectorType<MapVectorization::Scalar> {
    using Type = Bits_32;
    using MaskType = Bool;
    using KeyType = Bits_32;
    using InputHash = Bits_32;
    using SlotType = SlotEntryOnly;
  };

  using vectorize_type = VectorType<vector_mode>::Type;
  using mask_type = VectorType<vector_mode>::MaskType;
  using vector_key_type = VectorType<vector_mode>::KeyType;
  using slot_type = VectorType<vector_mode>::SlotType;
  using input_hash_type = VectorType<vector_mode>::InputHash;
  static constexpr Count bucket_size = sizeof(vectorize_type);
  static constexpr Count slot_count =
      vector_mode == MapVectorization::Scalar ? 1 : bucket_size;

  // Number of elements per bucket to hold back.
  // Holding back 2 elements results in a load factor of 0.9375 for wide
  // and a load factor of 0.875 for narrow.
  static constexpr Count load_factor = bucket_size - 2;

  struct BufferData {
    vectorize_type* bucket_buffer = nullptr;
    slot_type* slots_buffer = nullptr;
    Bits_32 bucket_count = 0;
    Bits_32 size = 0;
    Count total_byte_capacity = 0;
  };

 public:
  Map() { buffer_data = BufferData(); }
  Map(Count initial_capacity) { ensure_capacity(initial_capacity); }

  template <Count aggregate_size>
  constexpr Map(const Entry (&items)[aggregate_size]) {
    ensure_capacity(aggregate_size);

    for (Count i = 0; i < aggregate_size; i++) {
      insert(items[i]);
    }
  }

  Map(const Map& rhs) {
    if (rhs.buffer_data.bucket_count == 0) {
      buffer_data = BufferData();
      return;
    }

    buffer_data = create_buffer(rhs.buffer_data.bucket_count);
    buffer_data.size = rhs.buffer_data.size;

    if constexpr (
        __is_trivially_copyable(key_type) &&
        __is_trivially_copyable(value_type)) {
      memcpy(
          buffer_data.bucket_buffer,
          rhs.buffer_data.bucket_buffer,
          required_buffer_size(rhs.buffer_data.bucket_count));
      return;
    }

    memcpy(
        buffer_data.bucket_buffer,
        rhs.buffer_data.bucket_buffer,
        get_header_size(rhs.buffer_data.bucket_count));

    for (Count bucket_index = 0; bucket_index < buffer_data.bucket_count;
         bucket_index++) {
      auto occupancy_bits =
          occupied_slots(rhs.buffer_data.bucket_buffer[bucket_index]);
      auto occupancy_count = count_occupied(occupancy_bits);
      for (Count index = 0; index < occupancy_count; index++) {
        auto target_slot = slot_at(buffer_data, bucket_index, index);
        auto source_slot = slot_at(rhs.buffer_data, bucket_index, index);

        store_hash(
            target_slot,
            stored_hash(rhs.buffer_data, bucket_index, source_slot));
        new (&target_slot->entry)
            Entry(source_slot->entry.key, source_slot->entry.value);
      }
    }
  }

  Map(Map&& rhs) {
    buffer_data = rhs.buffer_data;

    rhs.buffer_data = BufferData();
  };

  ~Map() {
    if (buffer_data.bucket_buffer) {
      destruct();
      Core::Bibliotheca::remit((Bits_8*)buffer_data.bucket_buffer);
    }
  }

  auto ensure_capacity(Count items) -> void {
    if constexpr (vector_mode == MapVectorization::Scalar) {
      if (buffer_data.bucket_buffer &&
          items <= buffer_data.bucket_count * 0.9) {
        return;
      }

      auto new_bucket_count =
          buffer_data.bucket_count == 0 ? 2 : buffer_data.bucket_count << 1;
      while (new_bucket_count * 0.9 <= items) {
        new_bucket_count <<= 1;
      }

      rehash(new_bucket_count);
    } else {
      const auto load_limit = load_factor * buffer_data.bucket_count;
      if (items <= load_limit) {
        return;
      }
      auto new_bucket_count =
          buffer_data.bucket_count == 0 ? 1 : buffer_data.bucket_count << 1;
      while (load_factor * new_bucket_count <= items) {
        new_bucket_count <<= 1;
      }

      rehash(new_bucket_count);
    }
  }

  auto clear() -> void {
    destruct();

    buffer_data.size = 0;
  }

  constexpr auto insert(const Entry& item) -> Entry* {
    return insert(item.key, item.value);
  }

  constexpr auto insert(const key_type& key, const value_type& value)
      -> Entry* {
    ensure_capacity(buffer_data.size + 1);

    // If the entry already exists then overwrite the value.
    auto entry = find(key);
    if (entry) {
      entry->value = value;
      return entry;
    }

    auto hash = get_hash(key);
    auto empty_slot = get_empty(hash);
    buffer_data.size += 1;

    store_hash(empty_slot, hash);

    // Construct using the copy constructor.
    new (&empty_slot->entry) Entry(key, value);

    return &empty_slot->entry;
  }

  constexpr auto emplace(Entry&& item) -> Entry* {
    return emplace(item.key, item.value);
  }

  constexpr auto emplace(key_type&& key, value_type&& value) -> Entry* {
    ensure_capacity(buffer_data.size + 1);

    // If the entry already exists then overwrite the value.
    auto entry = find(key);
    if (entry) {
      entry->value = value;
      return entry;
    }

    auto hash = get_hash(key);
    auto empty_slot = get_empty(hash);
    buffer_data.size += 1;

    store_hash(empty_slot, hash);

    // Construct using the copy constructor.
    new (&empty_slot->entry) Entry(key, value);

    return &empty_slot->entry;
  }

  constexpr auto find(const key_type& key) -> Entry* {
    return const_cast<Entry*>(static_cast<const Map*>(this)->find(key));
  }

  constexpr auto find(const key_type& key) const -> const Entry* {
    auto slot = find_slot(key);
    return slot ? &slot->entry : nullptr;
  }

  constexpr auto contains(const key_type& key) const -> Bool {
    return find(key) != nullptr;
  }

  auto remove(const key_type& key) -> Bool {
    auto slot = find_slot(key);
    if (!slot) {
      return False;
    }

    Count slot_offset = Count(slot - buffer_data.slots_buffer);
    Count bucket_index = slot_offset / slot_count;
    Count slot_index = slot_offset % slot_count;
    Bool was_full =
        full_block(occupied_slots(buffer_data.bucket_buffer[bucket_index]));

    remove_slot(bucket_index, slot_index);
    if (was_full) {
      // A full bucket may have pushed entries into later buckets. Rehash after
      // compacting the local bucket so those probe chains rebuild from home.
      rehash(buffer_data.bucket_count);
    }

    return True;
  }

  auto get_entry(Count index) -> Entry* {
    return const_cast<Entry*>(
        static_cast<const Map*>(this)->get_entry(index));
  }

  auto get_entry(Count index) const -> const Entry* {
    if (index >= buffer_data.size) {
      return nullptr;
    }

    Count entry_offset = index;
    for (Count bucket_index = 0; bucket_index < buffer_data.bucket_count;
         bucket_index++) {
      auto occupancy_bits =
          occupied_slots(buffer_data.bucket_buffer[bucket_index]);
      if (!occupancy_bits) {
        continue;
      }

      Count occupancy_count = count_occupied(occupancy_bits);
      if (entry_offset < occupancy_count) {
        return &slot_at(buffer_data, bucket_index, entry_offset)->entry;
      }
      entry_offset -= occupancy_count;
    }

    return nullptr;
  }

  constexpr auto at(const key_type& key) -> value_type& {
    auto entry = find(key);
    if (!entry) {
      ensure_capacity(buffer_data.size + 1);

      auto hash = get_hash(key);
      auto empty_slot = get_empty(hash);
      buffer_data.size += 1;

      store_hash(empty_slot, hash);

      // Construct using the copy constructor.
      new (&empty_slot->entry) Entry(key, value_type());

      entry = &empty_slot->entry;
    }

    // Return end block
    return entry->value;
  }

  constexpr auto operator[](const key_type& key) -> value_type& {
    return at(key);
  }

  constexpr auto find_or_default(const key_type& key, const value_type& value)
      const -> const value_type& {
    auto entry = find(key);
    if (!entry) {
      return value;
    } else {
      return entry->value;
    }
  }

  constexpr auto get_size() const -> Count { return buffer_data.size; }
  constexpr auto get_capacity() const -> Count {
    return buffer_data.bucket_count * slot_count;
  }

 private:
  constexpr auto find_slot(const key_type& key) -> slot_type* {
    return const_cast<slot_type*>(
        static_cast<const Map*>(this)->find_slot(key));
  }

  // Shared lookup primitive for find and remove. Mutating callers can derive
  // the bucket index from the returned slot instead of carrying a side object.
  constexpr auto find_slot(const key_type& key) const -> const slot_type* {
    if (buffer_data.size == 0) {
      return nullptr;
    }

    auto hash = get_hash(key);
    auto bi = extract_vector_index(hash);
    auto vi = extract_vector_key(hash);
    auto buckets = buffer_data.bucket_buffer;
    auto bucket_count = buffer_data.bucket_count;

    while (true) {
      auto occupancy_bits = occupied_slots(buckets[bi]);
      auto possible_matches = extract_possible_matches(buckets[bi], vi);

      while (possible_matches) {
        Count index = first_match(possible_matches);
        auto target_slot = slot_at(buffer_data, bi, index);
        if (hash_matches(hash, target_slot) && target_slot->entry.key == key) {
          return target_slot;
        }

        clear_match(possible_matches, index);
      }

      if (!full_block(occupancy_bits)) {
        return nullptr;
      }

      bi = (bi + 1) & (bucket_count - 1);
    }
  }

  // Gets an empty bucket for a hash and set it as used.
  constexpr auto get_empty(const input_hash_type hash) -> slot_type* {
    auto bi = extract_vector_index(hash);
    auto vi = extract_vector_key(hash);
    auto buckets = buffer_data.bucket_buffer;
    auto bucket_count = buffer_data.bucket_count;

    // If a bucket happens to be full move to the next one.
    auto occupancy_bits = occupied_slots(buckets[bi]);
    while (full_block(occupancy_bits)) [[unlikely]] {
      bi = (bi + 1) & (bucket_count - 1);
      occupancy_bits = occupied_slots(buckets[bi]);
    }

    Count occupancy_count = count_occupied(occupancy_bits);
    set_slot_key(bi, occupancy_count, vi);
    return slot_at(buffer_data, bi, occupancy_count);
  }

  // Optimized placement of keys into the table when it is known the key does
  // not already exist in the table and that adding the key won't push past the
  // load factor limit.
  auto emplace_hashed(slot_type* slot, const input_hash_type hash) {
    auto empty_slot = get_empty(hash);

    // Copy over the element, ignoring any construction or destruction.
    memcpy(Core::Data::cast<void>(empty_slot), slot, sizeof(slot_type));
  }

  static constexpr auto slot_at(
      BufferData& data,
      Count bucket_index,
      Count slot_index) -> slot_type* {
    return data.slots_buffer + (bucket_index * slot_count) + slot_index;
  }

  static constexpr auto slot_at(
      const BufferData& data,
      Count bucket_index,
      Count slot_index) -> const slot_type* {
    return data.slots_buffer + (bucket_index * slot_count) + slot_index;
  }

  static constexpr auto count_occupied(mask_type occupancy_bits) -> Count {
    if constexpr (vector_mode == MapVectorization::Scalar) {
      return occupancy_bits ? 1 : 0;
    } else {
      return __builtin_popcountg(occupancy_bits);
    }
  }

  auto set_slot_key(
      Count bucket_index,
      Count slot_index,
      vector_key_type key) -> void {
    if constexpr (vector_mode == MapVectorization::Scalar) {
      buffer_data.bucket_buffer[bucket_index] = key;
    } else {
      Core::Data::cast<Bits_8>(buffer_data.bucket_buffer + bucket_index)
          [slot_index] = key;
    }
  }

  auto advance(
      Count bucket_index,
      Count target_index,
      Count source_index) -> void {
    if constexpr (vector_mode != MapVectorization::Scalar) {
      Bits_8* bucket_bytes =
          Core::Data::cast<Bits_8>(buffer_data.bucket_buffer + bucket_index);
      bucket_bytes[target_index] = bucket_bytes[source_index];
    }
  }

  auto clear_slot(Count bucket_index, Count slot_index) -> void {
    if constexpr (vector_mode == MapVectorization::Scalar) {
      clear_bucket(buffer_data.bucket_buffer + bucket_index);
    } else {
      Core::Data::cast<Bits_8>(buffer_data.bucket_buffer + bucket_index)
          [slot_index] = 0;
    }
  }

  auto store_hash(slot_type* slot, input_hash_type hash) -> void {
    if constexpr (vector_mode != MapVectorization::Scalar) {
      slot->hash = hash;
    }
  }

  static auto stored_hash(
      const BufferData& data,
      Count bucket_index,
      const slot_type* slot) -> input_hash_type {
    if constexpr (vector_mode == MapVectorization::Scalar) {
      return input_hash_type(data.bucket_buffer[bucket_index]);
    } else {
      return input_hash_type(slot->hash);
    }
  }

  static constexpr auto first_match(mask_type possible_matches) -> Count {
    if constexpr (vector_mode == MapVectorization::Scalar) {
      return 0;
    } else {
      return __builtin_ctzg(possible_matches);
    }
  }

  static constexpr auto clear_match(
      mask_type& possible_matches,
      Count index) -> void {
    if constexpr (vector_mode == MapVectorization::Scalar) {
      possible_matches = False;
    } else {
      possible_matches ^= mask_type(1) << index;
    }
  }

  static constexpr auto hash_matches(
      input_hash_type hash,
      const slot_type* slot) -> Bool {
    if constexpr (vector_mode == MapVectorization::Scalar) {
      return True;
    } else {
      return slot->hash == hash;
    }
  }

  auto remove_slot(Count bucket_index, Count slot_index) -> void {
    auto occupancy_bits =
        occupied_slots(buffer_data.bucket_buffer[bucket_index]);
    Count last_slot_index = count_occupied(occupancy_bits) - 1;
    slot_type* removed_slot = slot_at(buffer_data, bucket_index, slot_index);
    slot_type* dead_slot =
        slot_at(buffer_data, bucket_index, last_slot_index);

    // Keep each bucket packed. If this bucket was full, remove() rehashes after
    // compaction so any longer probe chain is rebuilt from home.
    if (slot_index != last_slot_index) {
      Core::Data::swap(*removed_slot, *dead_slot);
      advance(bucket_index, slot_index, last_slot_index);
    }

    clear_slot(bucket_index, last_slot_index);
    destruct_slot(dead_slot);
    buffer_data.size--;
  }

  auto destruct_slot(slot_type* slot) -> void {
    slot->entry.key.~key_type();
    slot->entry.value.~value_type();
  }

  auto destruct() -> void {
    // Look over all slots and destruct the keys and values.
    auto buckets = buffer_data.bucket_buffer;
    auto bucket_count = buffer_data.bucket_count;

    for (Count bucket_index = 0; bucket_index < bucket_count; bucket_index++) {
      auto occupancy_bits = occupied_slots(buckets[bucket_index]);

      if (!occupancy_bits) {
        continue;
      }

      auto occupancy_count = count_occupied(occupancy_bits);
      for (Count bit_index = 0; bit_index < occupancy_count; bit_index++) {
        destruct_slot(slot_at(buffer_data, bucket_index, bit_index));
      }

      // Clear the bucket.
      clear_bucket(buckets + bucket_index);
    }
  }

  auto rehash(const Count new_bucket_count) -> void {
    // Store the old values to clone.
    auto current_buffer = buffer_data;

    // Get a fresh block
    buffer_data = create_buffer(new_bucket_count);

    // Rehash
    for (Count bucket_index = 0; bucket_index < current_buffer.bucket_count;
         bucket_index++) {
      auto occupancy_bits =
          occupied_slots(current_buffer.bucket_buffer[bucket_index]);

      // If the mask is empty then move to the next block.
      if (!occupancy_bits) {
        continue;
      }

      auto occupancy_count = count_occupied(occupancy_bits);
      // Rehash each element into the new bucket.
      for (Count entry_index = 0; entry_index < occupancy_count;
           entry_index++) {
        auto valid_slot = slot_at(current_buffer, bucket_index, entry_index);
        emplace_hashed(
            valid_slot,
            stored_hash(current_buffer, bucket_index, valid_slot));
      }
    }

    // Remit the old block.
    if (current_buffer.bucket_buffer) {
      Core::Bibliotheca::remit((Bits_8*)current_buffer.bucket_buffer);
    }
    buffer_data.size = current_buffer.size;
  }

  constexpr auto get_hash(const key_type& key) const -> input_hash_type {
    Core::Hash h(key);
    return input_hash_type(h.get_value());
  }

  // Shift out the 7 bits used for the key.
  constexpr auto extract_vector_index(input_hash_type hash_key) const
      -> input_hash_type {
    if constexpr (vector_mode == MapVectorization::Scalar) {
      return hash_key & (buffer_data.bucket_count - 1);
    } else {
      return (hash_key >> 7) & (buffer_data.bucket_count - 1);
    }
  }

  // For AVX the highest bit is useful as a control bit so we reserve it and
  // only use 7 bits for the actual key.
  static constexpr auto extract_vector_key(input_hash_type hash_key)
      -> vector_key_type {
    if constexpr (vector_mode == MapVectorization::Scalar) {
      return vector_key_type(hash_key) | Bits_32(0x80000000);
    } else {
      return vector_key_type(hash_key) | 0b10000000;
    }
  }

  // Returns a bit mask of all possible slots.
  static constexpr auto extract_possible_matches(
      vectorize_type bucket,
      vector_key_type vi) -> mask_type {
    if constexpr (vector_mode == MapVectorization::Scalar) {
      return bucket == vi;
    } else if constexpr (vector_mode == MapVectorization::Partial) {
      const auto test_vector = _mm_set1_epi8(vi);
      const auto mask = _mm_cmpeq_epi8(bucket, test_vector);
      return mask_type(_mm_movemask_epi8(mask));
    } else {
      const auto test_vector = _mm256_set1_epi8(vi);
      const auto mask = _mm256_cmpeq_epi8(bucket, test_vector);
      return mask_type(_mm256_movemask_epi8(mask));
    }
  }

  // Use the control bit of every byte as the occupancy mask.
  static constexpr auto occupied_slots(vectorize_type bucket) -> mask_type {
    if constexpr (vector_mode == MapVectorization::Scalar) {
      return bucket != 0;
    } else if constexpr (vector_mode == MapVectorization::Partial) {
      return mask_type(_mm_movemask_epi8(bucket));
    } else {
      return mask_type(_mm256_movemask_epi8(bucket));
    }
  }

  // The block is full if every bit in the occupancy mask is set.
  static constexpr auto full_block(mask_type occupancy_bits) -> Bool {
    return occupancy_bits == mask_type(-1);
  }

  static constexpr auto clear_bucket(vectorize_type* bucket) -> void {
    if constexpr (vector_mode == MapVectorization::Scalar) {
      *bucket = 0;
    } else if constexpr (vector_mode == MapVectorization::Partial) {
      *bucket = _mm_setzero_pd();
    } else {
      *bucket = _mm256_setzero_pd();
    }
  }

  constexpr static auto get_header_size(Count bucket_count) -> Count {
    return bucket_count * bucket_size;
  }

  constexpr static auto required_buffer_size(Count buckets) -> Count {
    // We need 1 byte per entry so the number of header bytes maps directly to
    // capacity.
    auto header_size = buckets * bucket_size;

    // Actual data including the hash info.
    auto buffer_size = sizeof(slot_type) * buckets * slot_count;

    return header_size + buffer_size;
  }

  static auto create_buffer(Count buckets) -> BufferData {
    BufferData new_buffer;
    new_buffer.bucket_count = buckets;
    const auto required_size = required_buffer_size(new_buffer.bucket_count);
    const auto alloc = Core::Bibliotheca::check_out(required_size);

    // Store the total capacity in bytes for buffer copies later.
    new_buffer.total_byte_capacity = alloc.capacity;

    // Blocks from the Bibliotheca are always 64 byte aligned.
    new_buffer.bucket_buffer = Core::Data::cast<vectorize_type>(alloc.ptr);
    // slot_type data is everything directly after the bucket count.
    // This keeps us 32 byte aligned which should be good for any types.
    new_buffer.slots_buffer =
        Core::Data::cast<slot_type>(new_buffer.bucket_buffer + buckets);

    for (Count i = 0; i < buckets; i++) {
      clear_bucket(new_buffer.bucket_buffer + i);
    }

    return new_buffer;
  }

  BufferData buffer_data;
};

}  // namespace Perimortem::Memory::Dynamic
