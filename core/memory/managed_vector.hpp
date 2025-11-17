// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "core/memory/arena.hpp"

namespace Perimortem::Memory {

// A simple linear flat array of trivially constructable values.
template <typename T>
class ManagedVector {
 public:
  static constexpr uint32_t start_capacity = 16;
  static constexpr uint32_t growth_factor = 2;

  ManagedVector(const ManagedVector&) = default;
  ManagedVector(Arena& arena) : arena(&arena) { reset(); }

  auto reset() -> void {
    size = 0;
    capacity = start_capacity;
    rented_block = reinterpret_cast<T*>(
        arena->allocate(sizeof(T) * start_capacity, alignof(T)));
  }

  auto apply(const std::function<void(const T*)>& fn) const -> void {
    for (int i = 0; i < size; i++) {
      fn(rented_block[i]);
    }
  }

  constexpr auto insert(const T& data) -> void {
    if (size == capacity)
      grow();

    rented_block[size++] = data;
  }

  constexpr auto contains(const T& data) const -> bool {
    for (int i = 0; i < size; i++) {
      if (rented_block[i] == data) {
        return true;
      }
    }

    return false;
  }

  constexpr auto at(uint32_t index) const -> T& {
    return rented_block[index];
  }

  constexpr auto get_size() const -> uint32_t { return size; };

 private:
  auto grow() -> void {
    capacity *= growth_factor;
    auto new_block =
        reinterpret_cast<T*>(arena->allocate(sizeof(T) * capacity, alignof(T)));

    std::memcpy(new_block, rented_block, sizeof(T) * size);
    rented_block = new_block;
  }

  Arena* arena;
  T* rented_block;
  uint32_t size;
  uint32_t capacity;
};

}  // namespace Perimortem::Memory
