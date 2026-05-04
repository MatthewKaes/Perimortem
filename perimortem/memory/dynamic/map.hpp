// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data.hpp"
#include "perimortem/core/hash.hpp"
#include "perimortem/memory/allocator/bibliotheca.hpp"
#include "perimortem/utility/pair.hpp"

#include <x86intrin.h>

namespace Perimortem::Memory::Dynamic {

enum class MapVectorization { Full, Partial, Scalar };

// Unordered flat map with a two stage look up to optimize for insert, delete
// and find operations. Optimized for value type
//
// The table is optimized for the AMD Zen architecture and will most likely
// perform slightly worse than absl::flat_hash_map and boost::unordered_flap_map
// on Intel processors (I haven't tested but based on Anger's data up to
// TigerLake the latency points to boost's favor).
//
// The table is also able to optimize it's throughput on text keys by supporting
// Perimortem's hash function which is optimized for text keys to shave off
// cycles, losing performance mostly on sparse bit hashes.
//
// Since Perimortem uses 32 byte wide AVX2 registers the buckets are filled up
// linearly both for improved instruction throughput (and very minor branch
// performance for iteration) and to improve cache performance for small key /
// value pairs, although any major gains are offset by storing the 8 hash
// inline.
template <typename key_type,
          typename value_type,
          MapVectorization vector_mode = MapVectorization::Partial>
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

  // Number of elements per bucket to hold back.
  // Holding back 2 elements results in a load factor of 0.9375 for wide
  // and a load factor of 0.875 for narrow.
  static constexpr Count load_factor = bucket_size - 2;

  struct BufferData {
    Allocator::Bibliotheca::Preface* rented_block = nullptr;
    vectorize_type* bucket_buffer = nullptr;
    slot_type* slots_buffer = nullptr;
    Bits_32 bucket_count = 0;
    Bits_32 size = 0;
  };

 public:
  Map() { buffer_data = BufferData(); }
  Map(Count inital_capacity) { ensure_capacity(inital_capacity); }

  template <Count aggregate_size>
  constexpr Map(const Entry (&items)[aggregate_size]) {
    ensure_capacity(aggregate_size);

    for (Count i = 0; i < aggregate_size; i++) {
      insert(items[i]);
    }
  }

  Map(const Map& rhs) {
    buffer_data = create_buffer(rhs.buffer_data.bucket_count);
    buffer_data.size = rhs.buffer_data.size;

    memcpy(
        Allocator::Bibliotheca::preface_to_corpus(buffer_data.rented_block),
        Allocator::Bibliotheca::preface_to_corpus(rhs.buffer_data.rented_block),
        buffer_data.rented_block->get_usable_bytes());

    if (!__is_trivially_copyable(key_type) ||
        !__is_trivially_copyable(value_type)) {
      auto bucket_count = buffer_data.bucket_count;
      auto buckets = buffer_data.bucket_buffer;
      auto slots = buffer_data.slots_buffer;
      auto source_slots = rhs.buffer_data.slots_buffer;
      for (Count bucket_index = 0; bucket_index < bucket_count;
           bucket_index++) {
        auto occupancy_bits = occupied_slots(buckets[bucket_index]);

        if (!occupancy_bits) {
          continue;
        }

        auto occupancy_count = __builtin_popcountg(occupancy_bits);
        for (Count bit_index = 0; bit_index < occupancy_count; bit_index++) {
          auto target_slot = slots + (bucket_index * bucket_size) + bit_index;
          auto source_entry =
              source_slots + (bucket_index * bucket_size) + bit_index;
          target_slot->entry.key.key_type(source_slots->key);
          target_slot->entry.value.value_type(source_slots->value);
        }
      }
    }
  }

  Map(Map&& rhs) {
    buffer_data = rhs.buffer_data;

    rhs.buffer_data = BufferData();
  };

  ~Map() {
    if (buffer_data.rented_block) {
      destruct();
      Allocator::Bibliotheca::remit(buffer_data.rented_block);
    }
  }

  auto ensure_capacity(Count items) -> void {
    if constexpr (vector_mode == MapVectorization::Scalar) {
      if (items <= buffer_data.bucket_count * 0.9) {
        return;
      }

      auto new_bucket_count =
          buffer_data.bucket_count == 0 ? 2 : buffer_data.bucket_count << 1;
      while (new_bucket_count * 0.9 <= items) {
        new_bucket_count <<= 1;
      }

      grow(new_bucket_count);
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

      grow(new_bucket_count);
    }
  }

  auto reset() -> void {
    destruct();

    buffer_data.size = 0;
  }

  constexpr auto insert(const Entry& item) -> Entry* {
    return insert(item.key, item.value);
  }

  constexpr auto insert(const key_type& key, const value_type& value)
      -> Entry* {
    ensure_capacity(buffer_data.size + 1);

    // If the entry already exists than overwrite the value.
    auto entry = find(key);
    if (entry) {
      entry->value = value;
      return entry;
    }

    auto hash = get_hash(key);
    auto empty_slot = get_empty(hash);
    buffer_data.size += 1;

    // Scalar maps store the full hash in the bucket.
    if constexpr (vector_mode != MapVectorization::Scalar) {
      empty_slot->hash = hash;
    }

    // Construct using the copy constructor.
    new (&empty_slot->entry) Entry(key, value);

    return &empty_slot->entry;
  }

  constexpr auto emplace(Entry&& item) -> Entry* {
    return emplace(item.key, item.value);
  }

  constexpr auto emplace(key_type&& key, value_type&& value) -> Entry* {
    ensure_capacity(buffer_data.size + 1);

    // If the entry already exists than overwrite the value.
    auto entry = find(key);
    if (entry) {
      entry->value = value;
      return entry;
    }

    auto hash = get_hash(key);
    auto empty_slot = get_empty(hash);
    buffer_data.size += 1;

    // Scalar maps store the full hash in the bucket.
    if constexpr (vector_mode != MapVectorization::Scalar) {
      empty_slot->hash = hash;
    }

    // Construct using the copy constructor.
    new (&empty_slot->entry) Entry(key, value);

    return &empty_slot->entry;
  }

  constexpr auto contains(const key_type& key) const -> Bool {
    return find(key) != nullptr;
  }

  constexpr auto at(const key_type& key) -> value_type& {
    auto entry = find(key);
    if (!entry) {
      ensure_capacity(buffer_data.size + 1);

      auto hash = get_hash(key);
      auto empty_slot = get_empty(hash);
      buffer_data.size += 1;

      // Scalar maps store the full hash in the bucket.
      if constexpr (vector_mode != MapVectorization::Scalar) {
        empty_slot->hash = hash;
      }

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

  constexpr auto find_or_default(const key_type& key,
                                 const key_type& value) const
      -> const key_type& {
    auto entry = find(key);
    if (!entry) {
      return value;
    } else {
      return entry->value;
    }
  }

  constexpr auto get_size() const -> Count { return buffer_data.size; }
  constexpr auto get_capacity() const -> Count {
    if constexpr (vector_mode == MapVectorization::Scalar) {
      return buffer_data.bucket_count;
    } else {
      return buffer_data.bucket_count * bucket_size;
    }
  }
  constexpr auto get_memory_consumption() const -> Count {
    return Allocator::Bibliotheca::get_memory_consumption(
        buffer_data.rented_block);
  }

 private:
  // Get an entry if it exists.
  constexpr auto find(const key_type& key) const -> Entry* {
    if (buffer_data.size == 0) {
      return nullptr;
    }

    auto hash = get_hash(key);
    auto bi = extract_vector_index(hash);
    auto vi = extract_vector_key(hash);
    auto buckets = buffer_data.bucket_buffer;
    auto slots = buffer_data.slots_buffer;
    auto bucket_count = buffer_data.bucket_count;

    if constexpr (vector_mode == MapVectorization::Scalar) {
      auto possible_buckets = get_size();
      for (Count i = 0; i < possible_buckets; i++) {
        auto possible_match = extract_possible_matches(buckets[bi], vi);
        if (possible_match) {
          auto target_slot = slots + bi;
          if (target_slot->entry.key == key) {
            return &target_slot->entry;
          }
        } else if (!occupied_slots(buckets[bi])) {
          return nullptr;
        }

        // If jthe bucket is full then move to the next bucket.
        bi = (bi + 1) & (bucket_count - 1);
      }

      return nullptr;
    } else {
      while (true) {
        auto possible_matches = extract_possible_matches(buckets[bi], vi);

        while (possible_matches) {
          auto index = __builtin_ctzg(possible_matches);
          auto target_slot = slots + (bi * bucket_size) + index;
          if (target_slot->hash == hash && target_slot->entry.key == key) {
            return &target_slot->entry;
          }

          // Remove the incorrect match.
          possible_matches ^= 1 << index;
        }

        // If the bucket isn't full then the key isn't contained in the map.
        auto occupancy_bits = occupied_slots(buckets[bi]);
        if (!full_block(occupancy_bits))
          return nullptr;

        // If jthe bucket is full then move to the next bucket.
        bi = (bi + 1) & (bucket_count - 1);
      }
    }
  }

  // Gets an empty bucket for a hash and set it as used.
  constexpr auto get_empty(const input_hash_type hash) -> slot_type* {
    auto bi = extract_vector_index(hash);
    auto vi = extract_vector_key(hash);
    auto buckets = buffer_data.bucket_buffer;
    auto slots = buffer_data.slots_buffer;
    auto bucket_count = buffer_data.bucket_count;

    // If a bucket happens to be full move to the next one.
    auto occupancy_bits = occupied_slots(buckets[bi]);
    while (full_block(occupancy_bits)) [[unlikely]] {
      bi = (bi + 1) & (bucket_count - 1);
      occupancy_bits = occupied_slots(buckets[bi]);
    }

    if constexpr (vector_mode == MapVectorization::Scalar) {
      buckets[bi] = vi;
      return slots + bi;
    } else {
      auto occupancy_count = __builtin_popcountg(occupancy_bits);
      auto target_bucket = buckets + bi;
      reinterpret_cast<Byte*>(target_bucket)[occupancy_count] = vi;

      return slots + (bi * bucket_size) + occupancy_count;
    }
  }

  // Optimized placement of keys into the table when it is known the key does
  // not already exist in the table and that adding the key won't push past the
  // load factor limit.
  auto emplace_hashed(slot_type* slot, const input_hash_type hash) {
    auto empty_slot = get_empty(hash);

    // Copy over the element, ignoring and construction or destruction.
    memcpy(reinterpret_cast<void*>(empty_slot), slot, sizeof(slot_type));
  }

  auto destruct() -> void {
    // Look over all slots and destruct the keys and values.
    auto buckets = buffer_data.bucket_buffer;
    auto slots = buffer_data.slots_buffer;
    auto bucket_count = buffer_data.bucket_count;

    for (Count bucket_index = 0; bucket_index < bucket_count; bucket_index++) {
      auto occupancy_bits = occupied_slots(buckets[bucket_index]);

      if (!occupancy_bits) {
        continue;
      }

      if constexpr (vector_mode == MapVectorization::Scalar) {
        auto valid_slot = slots + bucket_index;
        valid_slot->entry.key.~key_type();
        valid_slot->entry.value.~value_type();
      } else {
        auto occupancy_count = __builtin_popcountg(occupancy_bits);
        for (Count bit_index = 0; bit_index < occupancy_count; bit_index++) {
          auto valid_slot = slots + (bucket_index * bucket_size) + bit_index;
          valid_slot->entry.key.~key_type();
          valid_slot->entry.value.~value_type();
        }
      }

      // Clear the bucket.
      clear_bucket(buckets + bucket_index);
    }
  }

  auto grow(const Count new_bucket_count) -> void {
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

      if constexpr (vector_mode == MapVectorization::Scalar) {
        auto valid_slot = current_buffer.slots_buffer + bucket_index;
        emplace_hashed(valid_slot, current_buffer.bucket_buffer[bucket_index]);
      } else {
        auto occupancy_count = __builtin_popcountg(occupancy_bits);
        // Rehash each element into the new bucket.
        for (Count entry_index = 0; entry_index < occupancy_count;
             entry_index++) {
          auto valid_slot = current_buffer.slots_buffer +
                            (bucket_index * bucket_size) + entry_index;

          emplace_hashed(valid_slot, valid_slot->hash);
        }
      }
    }

    // Remit the old block.
    if (current_buffer.rented_block) {
      Allocator::Bibliotheca::remit(current_buffer.rented_block);
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
  static constexpr auto extract_possible_matches(vectorize_type bucket,
                                                 vector_key_type vi)
      -> mask_type {
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
    if constexpr (vector_mode == MapVectorization::Scalar) {
      return occupancy_bits != 0;
    } else {
      return occupancy_bits == mask_type(-1);
    }
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

    // Acutal data including the hash info.
    auto buffer_size =
        sizeof(slot_type) * buckets *
        (vector_mode == MapVectorization::Scalar ? 1 : bucket_size);

    return header_size + buffer_size + /*alignment padding*/ bucket_size;
  }

  static auto create_buffer(Count buckets) -> BufferData {
    BufferData new_buffer;
    new_buffer.bucket_count = buckets;
    const auto required_size = required_buffer_size(new_buffer.bucket_count);
    new_buffer.rented_block = Allocator::Bibliotheca::check_out(required_size);

    // Caculate a valid bucket location with proper alignment.
    Byte* address =
        Allocator::Bibliotheca::preface_to_corpus(new_buffer.rented_block);
    address += bucket_size - (Count(address) % bucket_size);
    new_buffer.bucket_buffer = reinterpret_cast<vectorize_type*>(address);
    // slot_type data is everything directly after the bucket count.
    // This keeps us 32 byte aligned which should be good for any types.
    new_buffer.slots_buffer =
        reinterpret_cast<slot_type*>(address + buckets * bucket_size);

    for (Count i = 0; i < buckets; i++) {
      clear_bucket(new_buffer.bucket_buffer + i);
    }

    return new_buffer;
  }

  BufferData buffer_data;
};

}  // namespace Perimortem::Memory::Dynamic
