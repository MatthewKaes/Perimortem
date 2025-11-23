// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "core/memory/managed_string.hpp"

#include <cstring>
#include <string_view>
#include <functional>

namespace Perimortem::Memory {

// A simple linear look up table for associating managed names to a value.
template <typename T>
class ManagedLookup {
 public:
  static constexpr uint32_t start_capacity = 8;
  static constexpr uint32_t growth_factor = 2;

  struct Entry {
    ManagedString name;
    T* data;
  };

  ManagedLookup(const ManagedLookup&) = default;
  ManagedLookup(Arena& arena) : arena(&arena) { reset(); }

  auto reset() -> void {
    size = 0;
    capacity = start_capacity;
    rented_block = reinterpret_cast<Entry*>(
        arena->allocate(sizeof(Entry) * start_capacity));
  }

  auto apply(const std::function<void(const T*)>& fn) const -> void {
    for (int i = 0; i < size; i++) {
      fn(rented_block[i].data);
    }
  }

  constexpr auto insert(const ManagedString& name, T* data) -> void {
    if (size == capacity)
      grow();

    rented_block[size++] = {name, data};
  }

  constexpr auto contains(const ManagedString& name) const -> bool {
    for (int i = 0; i < size; i++) {
      if (rented_block[i].name == name) {
        return true;
      }
    }

    return false;
  }

  constexpr auto at(const ManagedString& name) const -> T* {
    for (int i = 0; i < size; i++) {
      if (rented_block[i].name == name) {
        return rented_block[i].data;
      }
    }

    return nullptr;
  }

  constexpr auto get_size() const -> uint32_t { return size; };

 private:
  auto grow() -> void {
    capacity *= growth_factor;
    auto new_block = reinterpret_cast<Entry*>(
        arena->allocate(sizeof(Entry) * capacity));

    std::memcpy(new_block, rented_block, sizeof(Entry) * size);
    rented_block = new_block;
  }

  Arena* arena;
  Entry* rented_block;
  uint32_t size;
  uint32_t capacity;
};

}  // namespace Perimortem::Memory
