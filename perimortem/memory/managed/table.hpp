// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/dynamic/record.hpp"
#include "perimortem/memory/view/bytes.hpp"
#include "perimortem/memory/view/table.hpp"

#include <functional>
#include <initializer_list>

namespace Perimortem::Memory::Managed {

// A simple linear look up table for associating managed names to a value.
template <typename T>
class Table {
 public:

  static constexpr Count start_capacity = 8;
  static constexpr Count growth_factor = 2;

  using Entry = View::Table<T>::Entry;

  Table(const Table&) = default;
  Table(Allocator::Arena& arena) : arena(arena) { reset(); }

  constexpr operator View::Table<T>() const {
    return View::Table<T>(rented_block, size);
  }

  auto reset() -> void {
    size = 0;
    capacity = start_capacity;
    rented_block = reinterpret_cast<Entry*>(
        arena.allocate(sizeof(Entry) * start_capacity));
  }

  auto apply(const std::function<void(const View::Bytes, const T&)>& fn) const
      -> void {
    for (Count i = 0; i < size; i++) {
      fn(rented_block[i].name, rented_block[i].data);
    }
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
  constexpr auto get_view() const -> View::Table<T> {
    return View::Table<T>(rented_block, size);
  }

 private:
  auto grow() -> void {
    capacity *= growth_factor;
    auto new_block =
        reinterpret_cast<Entry*>(arena.allocate(sizeof(Entry) * capacity));

    std::memcpy(reinterpret_cast<void*>(new_block),
                reinterpret_cast<void*>(rented_block), sizeof(Entry) * size);
    rented_block = new_block;
  }

  Allocator::Arena& arena;
  Entry* rented_block;
  Count size;
  Count capacity;
};

}  // namespace Perimortem::Memory::Managed
