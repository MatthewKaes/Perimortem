// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/bibliotheca.hpp"
#include "perimortem/memory/const/standard_types.hpp"

#include <cstring>

namespace Perimortem::Memory::Dynamic {

// Unordered hash map where the hashing function uses zero as a reserved value.
template <typename key_type, typename value_type>
class Map {
 public:
  struct Entry {
    Bits_64 value;
    key_type key;
    value_type data;
  };

  static constexpr Count start_capacity = 8;
  static constexpr Count growth_factor = 2;

  Map() {
    rented_block = Bibliotheca::check_out(start_capacity * sizeof(Entry));
    entries = re
  }
  Map(const Map&) {};
  Map(Map&&) {};

  auto reset() -> void {
    // Capacity must always be less than or equal to the existing capacity in
    // the block on reset.
    destruct();

    size = 0;
    capacity = start_capacity;
  }

  constexpr auto insert(const View::Bytes name, const T& data) -> void {
    if (size >= capacity)
      grow();

    // Construct using the copy constructor.
    new (rented_block + (size++)) Entry(name, data);
  }

  constexpr auto contains(const View::Bytes name) const -> bool {
    for (Count i = 0; i < size; i++) {
      if (rented_block[i].name == name) {
        return true;
      }
    }

    return false;
  }

  constexpr auto at(const View::Bytes name) -> T& {
    for (Count i = 0; i < size; i++) {
      if (rented_block[i].name == name) {
        return rented_block[i].data;
      }
    }

    // Return end block
    return rented_block[capacity - 1].data;
  }

  constexpr auto at(Count index) const -> Entry& { return rented_block[index]; }

  constexpr auto operator[](const View::Bytes name) -> T& { return at(name); }

  constexpr auto operator[](Count index) -> Entry& { return at(index); }

  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_arena() const -> Allocator::Arena& { return arena; }
  // constexpr auto get_view() const -> View::Table<T> {
  //   return View::Table<T>(rented_block, size);
  // }

 private:
  auto destruct() -> void {
    // Look over all entries and destruct the keys and values.
    for (Count i = 0; i < capacity; i++) {
      if (entries[i].hash != 0) {
        entries[i].hash = 0;
        entries[i].key.~key_type();
        entries[i].value.~value_type();
      }
    }
  }
  auto grow() -> void {
    capacity *= growth_factor;
    auto new_block =
        reinterpret_cast<Entry*>(arena.allocate(sizeof(Entry) * capacity));

    std::memcpy(reinterpret_cast<void*>(new_block),
                reinterpret_cast<void*>(rented_block), sizeof(Entry) * size);
    rented_block = new_block;
  }

  constexpr auto get_size() const -> Count { return size; }

  constexpr auto get_view() const -> const View::Bytes {
    return View::Bytes(access(), size);
  }

  constexpr operator View::Bytes() const { return get_view(); }

 private:
  Count size = 0;
};

}  // namespace Perimortem::Memory::Dynamic
