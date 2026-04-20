// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/bibliotheca.hpp"
#include "perimortem/memory/static/hash.hpp"

#include "perimortem/core/standard_types.hpp"

#include <x86intrin.h>

namespace Perimortem::Memory::Dynamic {

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
template <typename key_type, typename value_type>
class Map {
 public:
  struct Entry {
    key_type key;
    value_type data;
  };

  struct Slot {
    Bits_64 hash;
    Entry entry;
  };

  static constexpr Count bucket_size = 32;
  static constexpr Count start_capacity = 1;
  static constexpr Count growth_factor = 2;
  static constexpr Count load_factor = 90;

  Map() { rented_block = create_buffer(bucket_count); }

  Map(const Map& rhs) {
    size = rhs.size;
    bucket_count = rhs.bucket_count;
    rented_block = create_buffer(bucket_count);

    std::memcpy(Allocator::Bibliotheca::preface_to_corpus(rented_block),
                Allocator::Bibliotheca::preface_to_corpus(rhs.rented_block),
                rented_block->usable_bytes());

    if (!std::is_trivially_copyable<key_type>::value ||
        !std::is_trivially_copyable<value_type>::value) {
      auto buckets = access_buckets(rented_block);
      auto slots = access_slots(rented_block);
      auto source_slots = access_slots(rhs.rented_block);
      for (Count bucket_index = 0; bucket_index < bucket_count;
           bucket_index++) {
        auto occupancy_bits = occupied_slots(buckets[bucket_index]);

        auto occupancy_count = std::popcount(occupancy_bits);
        if (!occupancy_bits) {
          continue;
        }

        for (Count bit_index = 0; bit_index < occupancy_count; bit_index++) {
          auto target_slot =
              slots + (bucket_index * sizeof(__m256i)) + bit_index;
          auto source_entry =
              source_slots + (bucket_index * sizeof(__m256i)) + bit_index;
          target_slot->entry.key.key_type(source_slots->key);
          target_slot->entry.value.value_type(source_slots->value);
        }
      }
    }
  }

  Map(Map&& rhs) {
    size = rhs.size;
    bucket_count = rhs.bucket_count;
    rented_block = rhs.rented_block;

    rhs.size = 0;
    rhs.rented_block = nullptr;
  };

  ~Map() {
    if (rented_block) {
      destruct();
      Allocator::Bibliotheca::remit(rented_block);
    }
  }

  auto reset() -> void {
    destruct();

    size = 0;
  }

  constexpr auto insert(const key_type& key, const value_type& value)
      -> Entry* {
    size += 1;
    if (size * 100 / (bucket_count * bucket_size) >= load_factor) {
      grow();
    }

    // If the entry already exists than overwrite the value.
    auto entry = find(key);
    if (entry) {
      entry->value = value;
      return entry;
    }

    auto hash = get_hash(key);
    entry = get_empty(hash);

    entry->hash = hash;

    // Construct using the copy constructor.
    new (entry->key) key_type(key);
    new (entry->value) value_type(value);

    return entry;
  }

  constexpr auto contains(const key_type& key) const -> Bool {
    return find(key) != nullptr;
  }

  constexpr auto at(const key_type& key) -> value_type& {
    auto entry = find(key);
    if (!entry) {
      entry = insert(key, value_type());
    }

    // Return end block
    return entry->value;
  }

  constexpr auto operator[](const key_type& key) -> value_type& {
    return at(key);
  }

  constexpr auto get_size() const -> Count { return size; }

 private:
  // Get an entry if it exists.
  constexpr auto find(const key_type& key) const -> Entry* {
    auto hash = get_hash(key);
    auto bi = extract_vector_index(hash);
    auto vi = extract_vector_key(hash);
    auto buckets = access_buckets(rented_block);
    auto slots = access_slots(rented_block);

    while (true) {
      auto possible_matches = extract_possible_matches(buckets, vi);
      while (possible_matches) {
        auto index = std::countl_zero(possible_matches);
        auto target_slot = slots + (bi * sizeof(__m256i)) + index;
        if (target_slot->hash == hash && target_slot->key == key) {
          return target_slot;
        }

        // Remove the incorrect match.
        possible_matches ^= 1 << index;
      }

      // If the bucket isn't full then the key isn't contained in the map.
      auto occupancy_bits = occupied_slots(bi);
      if (!full_block(occupancy_bits))
        return nullptr;

      // If jthe bucket is full then move to the next bucket.
      bi = (bi + 1) & (bucket_count - 1);
    }
  }

  // Gets an empty bucket for a hash and set it as used.
  constexpr auto get_empty(const Bits_64 hash) const -> Entry* {
    auto buckets = access_buckets(rented_block);
    auto slots = access_slots(rented_block, bucket_count);
    auto bi = extract_vector_index(hash);
    auto vi = extract_vector_key(hash);

    // If a bucket happens to be full move to the next one.
    auto occupancy_bits = occupied_slots(bi);
    while (full_block(occupancy_bits)) [[unlikely]] {
      bi = (bi + 1) & (bucket_count - 1);
      occupancy_bits = occupied_slots(bi);
    }

    auto occupancy_count = std::popcount(occupancy_bits);
    reinterpret_cast<Byte*>(buckets + bi)[occupancy_count] = vi;

    return slots + (bi * sizeof(__m256i)) + occupancy_count;
  }

  // Optimized placement of keys into the table when it is known the key does
  // not already exist in the table and that adding the key won't push past the
  // load factor limit.
  auto emplace_hashed(Entry* entry) {
    auto new_entry = get_empty(entry->hash);

    // Copy over the element, ignoring and construction or destruction.
    std::memcpy(new_entry, entry, sizeof(Entry));
  }

  auto destruct() -> void {
    // Look over all slots and destruct the keys and values.
    auto buckets = access_buckets(rented_block);
    auto slots = access_slots(rented_block);
    for (Count bucket_index = 0; bucket_index < bucket_count; bucket_index++) {
      auto occupancy_bits = occupied_slots(buckets[bucket_index]);

      auto occupancy_count = std::popcount(occupancy_bits);
      if (!occupancy_bits) {
        continue;
      }

      for (Count bit_index = 0; bit_index < occupancy_count; bit_index++) {
        auto valid_slot =
            slots + (bucket_index * sizeof(__m256i)) + bit_index;
        valid_slot->entry.key.~key_type();
        valid_slot->entry.value.~value_type();
      }

      // Clear the bucket.
      clear_bucket(buckets + bucket_index);
    }
  }

  auto grow() -> void {
    // Store the old values to clone.
    auto current_bucket_count = bucket_count;
    auto current_block = rented_block;
    auto current_buckets = access_buckets(current_block);
    auto current_slots = access_slots(current_block, bucket_count);

    // Get a fresh block
    bucket_count *= growth_factor;
    rented_block = create_buffer(bucket_count);

    // Rehash
    for (Count bucket_index = 0; bucket_index < current_bucket_count;
         bucket_index++) {
      auto occupancy_bits = occupied_slots(current_buckets[bucket_index]);
      auto occupancy_count = std::popcount(occupancy_bits);

      // If the mask is empty then move to the next block.
      if (!occupancy_bits) {
        continue;
      }

      // Rehash each element into the new bucket.
      for (Count entry_index = 0; entry_index < occupancy_count;
           entry_index++) {
        auto valid_slot =
            current_slots + (bucket_index * sizeof(__m256i)) + entry_index;

        emplace_hashed(valid_slot);
      }
    }

    // Remit the old block.
    Allocator::Bibliotheca::remit(current_block);
  }

  constexpr auto get_hash(const key_type& key) {
    Static::Hash h(key);
    return h.get_value() &
           0b01111111'11111111'11111111'11111111'11111111'11111111'11111111'11111111ULL;
  }
  // Shift out the 7 bits used for the key.
  constexpr auto extract_vector_index(Bits_64 hash_key) const -> Bits_64 {
    return (hash_key >> 7) & (bucket_count - 1);
  }

  // For AVX the highest bit is useful as a control bit so we reserve it and
  // only use 7 bits for the actual key.
  constexpr auto extract_vector_key(Bits_64 hash_key) const -> Bits_8 {
    return static_cast<Bits_8>(hash_key) | 0b10000000;
  }

  // Returns a bit mask of all possible slots.
  constexpr auto extract_possible_matches(__m256i bucket, Byte vi) -> Bits_32 {
    const auto test_vector = _mm256_set1_epi8(vi);
    const auto mask = _mm256_cmpeq_epi8(bucket, test_vector);
    return _mm256_movemask_epi8(mask);
  }

  // Use the control bit of every byte as the occupancy mask.
  constexpr auto occupied_slots(__m256i bucket) -> Bits_32 {
    return _mm256_movemask_epi8(bucket);
  }

  // The block is full if every bit in the occupancy mask is set.
  constexpr auto full_block(Bits_32 occupancy_bits) -> Bool {
    return occupancy_bits == 0b11111111'11111111'11111111'11111111U;
  }

  static constexpr auto access_buckets(Allocator::Bibliotheca::Preface* preface)
      -> __m256i* {
    return reinterpret_cast<__m256i*>(
        Allocator::Bibliotheca::preface_to_corpus(preface));
  }

  static constexpr auto clear_bucket(__m256i* bucket) -> void {
    *bucket = _mm256_setzero_pd();
  }

  static constexpr auto access_slots(Allocator::Bibliotheca::Preface* preface,
                                       Count bucket_count) -> Entry* {
    return reinterpret_cast<Entry*>(
        Allocator::Bibliotheca::preface_to_corpus(preface) +
        get_header_size(bucket_count));
  }

  constexpr static auto get_header_size(Count bucket_count) -> Count {
    return bucket_count * sizeof(__m256i);
  }

  constexpr static auto required_buffer_size(Count buckets) -> Count {
    // We need 1 byte per entry so the number of header bytes maps directly to
    // capacity.
    auto header_size = buckets * sizeof(__m256i);

    // Acutal data including the hash info.
    auto buffer_size = sizeof(Entry) * buckets * sizeof(__m256i);

    return header_size + buffer_size;
  }

  static auto create_buffer(Count blocks) -> Allocator::Bibliotheca::Preface* {
    auto* new_block =
        Allocator::Bibliotheca::check_out(required_buffer_size(blocks));

    auto new_buckets = access_buckets(new_block);
    for (Count i = 0; i < blocks; i++) {
      clear_bucket(new_buckets + i);
    }

    return new_block;
  }

  Count size = 0;
  Count bucket_count = start_capacity;
  Allocator::Bibliotheca::Preface* rented_block;
};

}  // namespace Perimortem::Memory::Dynamic
