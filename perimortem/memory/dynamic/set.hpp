// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/bibliotheca.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/hash.hpp"

namespace Perimortem::Memory::Dynamic {

// Unordered scalar hash set that owns keys by value.
//
// Keys live inline in the table and may be relocated by insert, remove, clear,
// or ensure_capacity. Pointers returned from find are short-lived inspection
// handles; do not keep them across any mutating call. For large objects or
// objects that need stable addresses, keep the object owned elsewhere and use a
// small pointer key object in the set.
template <typename key_type>
class Set {
 public:
  Set() = default;
  Set(Count initial_capacity) { ensure_capacity(initial_capacity); }

  Set(const Set& rhs) {
    ensure_capacity(rhs.get_size());
    for (Count bucket_index = 0; bucket_index < rhs.buffer_data.bucket_count;
         bucket_index++) {
      if (rhs.buffer_data.bucket_buffer[bucket_index] == 0) {
        continue;
      }

      insert(rhs.buffer_data.slots_buffer[bucket_index]);
    }
  }

  Set(Set&& rhs) {
    buffer_data = rhs.buffer_data;
    rhs.buffer_data = BufferData();
  }

  auto operator=(const Set& rhs) -> Set& {
    if (this == &rhs) {
      return *this;
    }

    clear();
    ensure_capacity(rhs.get_size());
    for (Count bucket_index = 0; bucket_index < rhs.buffer_data.bucket_count;
         bucket_index++) {
      if (rhs.buffer_data.bucket_buffer[bucket_index] == 0) {
        continue;
      }

      insert(rhs.buffer_data.slots_buffer[bucket_index]);
    }

    return *this;
  }

  auto operator=(Set&& rhs) -> Set& {
    if (this == &rhs) {
      return *this;
    }

    // Swap buffers so the donor destructor releases the block this set used to
    // own while this set takes over the donor's block.
    Core::Data::swap(buffer_data, rhs.buffer_data);
    return *this;
  }

  ~Set() {
    if (buffer_data.bucket_buffer == nullptr) {
      return;
    }

    destruct();
    Core::Bibliotheca::remit(
        Core::Data::cast<Bits_8>(buffer_data.bucket_buffer));
    buffer_data = BufferData();
  }

  auto ensure_capacity(Count items) -> void {
    if (buffer_data.bucket_buffer &&
        items * 10 <= Count(buffer_data.bucket_count) * 9) {
      return;
    }

    Count new_bucket_count =
        buffer_data.bucket_count == 0 ? 2 : buffer_data.bucket_count << 1;
    while (items * 10 > new_bucket_count * 9) {
      new_bucket_count <<= 1;
    }

    grow(new_bucket_count);
  }

  auto insert(const key_type& key) -> Bool {
    Bits_32 hash = get_hash(key);
    if (find_hashed(key, hash) != nullptr) {
      return False;
    }

    ensure_capacity(buffer_data.size + 1);
    key_type* empty_slot = get_empty(hash);
    buffer_data.size += 1;
    new (empty_slot) key_type(key);
    return True;
  }

  // Removal repairs the following probe cluster so lookup can still stop at the
  // first empty bucket without losing keys that were displaced by collisions.
  auto remove(const key_type& key) -> Bool {
    Count bucket_index = find_bucket(key, get_hash(key));
    if (bucket_index == Count(-1)) {
      return False;
    }

    Count bucket_mask = buffer_data.bucket_count - 1;
    Count next_bucket = (bucket_index + 1) & bucket_mask;
    Count hole_bucket = bucket_index;
    buffer_data.bucket_buffer[bucket_index] = 0;

    // The removed key object stays alive in the hole. Keep scanning until the
    // first empty bucket because an entry that is already in its home bucket may
    // be followed by another entry whose probe path still crosses the hole.
    while (buffer_data.bucket_buffer[next_bucket] != 0) {
      Bits_32 hash = buffer_data.bucket_buffer[next_bucket];
      Count home_bucket = extract_bucket_index(hash);

      // Both distances are measured from the entry's home bucket around the
      // circular table. If the hole is closer than the current slot, lookup
      // would stop at the hole before reaching this entry, so the entry has to
      // move back and the carried removed key object moves forward.
      Count entry_distance = (next_bucket - home_bucket) & bucket_mask;
      Count hole_distance = (hole_bucket - home_bucket) & bucket_mask;
      if (hole_distance < entry_distance) {
        buffer_data.bucket_buffer[hole_bucket] = hash;
        buffer_data.bucket_buffer[next_bucket] = 0;
        Core::Data::swap(
            buffer_data.slots_buffer[hole_bucket],
            buffer_data.slots_buffer[next_bucket]);
        hole_bucket = next_bucket;
      }

      next_bucket = (next_bucket + 1) & bucket_mask;
    }

    buffer_data.slots_buffer[hole_bucket].~key_type();
    buffer_data.size--;
    return True;
  }

  auto clear() -> void {
    destruct();
    buffer_data.size = 0;
  }

  // The returned pointer is only valid until the next mutating call.
  auto find(const key_type& key) -> key_type* {
    return const_cast<key_type*>(static_cast<const Set*>(this)->find(key));
  }

  auto find(const key_type& key) const -> const key_type* {
    return find_hashed(key, get_hash(key));
  }

  auto contains(const key_type& key) const -> Bool {
    return find(key) != nullptr;
  }

  template <typename visit_type>
  auto visit(visit_type visit_function) -> void {
    for (Count bucket_index = 0; bucket_index < buffer_data.bucket_count;
         bucket_index++) {
      if (buffer_data.bucket_buffer[bucket_index] == 0) {
        continue;
      }

      visit_function(buffer_data.slots_buffer[bucket_index]);
    }
  }

  template <typename visit_type>
  auto visit(visit_type visit_function) const -> void {
    for (Count bucket_index = 0; bucket_index < buffer_data.bucket_count;
         bucket_index++) {
      if (buffer_data.bucket_buffer[bucket_index] == 0) {
        continue;
      }

      const key_type& key = buffer_data.slots_buffer[bucket_index];
      visit_function(key);
    }
  }

  constexpr auto get_size() const -> Count { return buffer_data.size; }
  constexpr auto get_capacity() const -> Count {
    return buffer_data.bucket_count;
  }
  constexpr auto is_empty() const -> Bool { return buffer_data.size == 0; }

 private:
  struct BufferData {
    Bits_32* bucket_buffer = nullptr;
    key_type* slots_buffer = nullptr;
    Bits_32 bucket_count = 0;
    Bits_32 size = 0;
  };

  auto get_empty(Bits_32 hash) -> key_type* {
    Count bucket_index = extract_bucket_index(hash);
    while (buffer_data.bucket_buffer[bucket_index] != 0) {
      bucket_index = (bucket_index + 1) & (buffer_data.bucket_count - 1);
    }

    buffer_data.bucket_buffer[bucket_index] = extract_bucket_key(hash);
    return buffer_data.slots_buffer + bucket_index;
  }

  auto find_bucket(const key_type& key, Bits_32 hash) const -> Count {
    if (buffer_data.size == 0) {
      return Count(-1);
    }

    Bits_32 bucket_key = extract_bucket_key(hash);
    Count bucket_index = extract_bucket_index(hash);
    for (Count probe_count = 0; probe_count < buffer_data.bucket_count;
         probe_count++) {
      Bits_32 bucket = buffer_data.bucket_buffer[bucket_index];
      if (bucket == bucket_key &&
          buffer_data.slots_buffer[bucket_index] == key) {
        return bucket_index;
      }

      if (bucket == 0) {
        return Count(-1);
      }

      bucket_index = (bucket_index + 1) & (buffer_data.bucket_count - 1);
    }

    return Count(-1);
  }

  auto find_hashed(const key_type& key, Bits_32 hash) const -> const key_type* {
    Count bucket_index = find_bucket(key, hash);
    if (bucket_index == Count(-1)) {
      return nullptr;
    }

    return buffer_data.slots_buffer + bucket_index;
  }

  auto emplace_hashed(key_type* slot, Bits_32 hash) -> void {
    key_type* empty_slot = get_empty(hash);
    memcpy(Core::Data::cast<void>(empty_slot), slot, sizeof(key_type));
  }

  auto destruct() -> void {
    if (buffer_data.bucket_buffer == nullptr) {
      return;
    }

    for (Count bucket_index = 0; bucket_index < buffer_data.bucket_count;
         bucket_index++) {
      if (buffer_data.bucket_buffer[bucket_index] == 0) {
        continue;
      }

      buffer_data.slots_buffer[bucket_index].~key_type();
      buffer_data.bucket_buffer[bucket_index] = 0;
    }
  }

  auto grow(Count new_bucket_count) -> void {
    BufferData current_buffer = buffer_data;
    buffer_data = create_buffer(new_bucket_count);

    for (Count bucket_index = 0; bucket_index < current_buffer.bucket_count;
         bucket_index++) {
      if (current_buffer.bucket_buffer[bucket_index] == 0) {
        continue;
      }

      emplace_hashed(
          current_buffer.slots_buffer + bucket_index,
          current_buffer.bucket_buffer[bucket_index]);
    }

    if (current_buffer.bucket_buffer != nullptr) {
      Core::Bibliotheca::remit(
          Core::Data::cast<Bits_8>(current_buffer.bucket_buffer));
    }
    buffer_data.size = current_buffer.size;
  }

  static auto create_buffer(Count buckets) -> BufferData {
    BufferData new_buffer;
    new_buffer.bucket_count = buckets;
    Core::Bibliotheca::Allocation allocation =
        Core::Bibliotheca::check_out(required_buffer_size(buckets));
    new_buffer.bucket_buffer = Core::Data::cast<Bits_32>(allocation.ptr);
    new_buffer.slots_buffer =
        Core::Data::cast<key_type>(allocation.ptr + slot_offset(buckets));

    for (Count bucket_index = 0; bucket_index < buckets; bucket_index++) {
      new_buffer.bucket_buffer[bucket_index] = 0;
    }

    return new_buffer;
  }

  static constexpr auto required_buffer_size(Count buckets) -> Count {
    return slot_offset(buckets) + sizeof(key_type) * buckets;
  }

  static constexpr auto slot_offset(Count buckets) -> Count {
    return Core::Data::align<alignof(key_type)>(sizeof(Bits_32) * buckets);
  }

  constexpr auto extract_bucket_index(Bits_32 hash) const -> Count {
    return hash & (buffer_data.bucket_count - 1);
  }

  static constexpr auto extract_bucket_key(Bits_32 hash) -> Bits_32 {
    return hash | Bits_32(0x80000000);
  }

  static auto get_hash(const key_type& key) -> Bits_32 {
    return Bits_32(Core::Hash(key).get_value());
  }

  BufferData buffer_data;
};

}  // namespace Perimortem::Memory::Dynamic
