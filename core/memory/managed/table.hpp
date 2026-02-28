// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "memory/view/bytes.hpp"
#include "memory/view/table.hpp"

#include <functional>
#include <initializer_list>

namespace Perimortem::Memory::Managed {

// A simple linear look up table for associating managed names to a value.
template <typename T>
class Table {
 public:
  static constexpr uint32_t start_capacity = 8;
  static constexpr uint32_t growth_factor = 2;

  using Entry = View::Table<T>::Entry;

  Table(const Table&) = default;
  Table(Arena& arena) : arena(arena) { reset(); }

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
    for (uint32_t i = 0; i < size; i++) {
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
    for (uint32_t i = 0; i < size; i++) {
      if (rented_block[i].name == name) {
        return true;
      }
    }

    return false;
  }

  constexpr auto at(const View::Bytes name) -> T& {
    for (uint32_t i = 0; i < size; i++) {
      if (rented_block[i].name == name) {
        return &rented_block[i].data;
      }
    }

    // Return end block
    return &rented_block[capacity - 1].data;
  }

  constexpr auto at(uint32_t index) const -> Entry& {
    return rented_block[index];
  }

  constexpr auto operator[](const View::Bytes name) -> T& { return at(name); }

  constexpr auto operator[](uint32_t index) -> Entry& { return at(index); }

  constexpr auto get_size() const -> uint32_t { return size; }
  constexpr auto get_arena() const -> Arena& { return arena; }
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

  Arena& arena;
  Entry* rented_block;
  uint32_t size;
  uint32_t capacity;
};

}  // namespace Perimortem::Memory::Managed
