// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "memory/arena.hpp"
#include "memory/view/vector.hpp"

#include <cstring>

namespace Perimortem::Memory::Managed {

// A simple linear flat array of trivially constructable values.
template <typename T>
class Vector {
 public:
  static constexpr uint32_t start_capacity = 8;
  static constexpr uint32_t growth_factor = 2;

  Vector(const Vector&) = default;
  Vector(Arena& arena) : arena(arena) { reset(); }

  constexpr operator View::Vector<T>() const {
    return View::Vector<T>(rented_block, size);
  }

  auto reset() -> void {
    size = 0;
    capacity = start_capacity;
    rented_block =
        reinterpret_cast<T*>(arena.allocate(sizeof(T) * start_capacity));
  }

  auto reset(uint32_t reserve_capacity) -> void {
    if (reserve_capacity <= start_capacity) {
      reserve_capacity = start_capacity;
    }

    size = 0;
    capacity = reserve_capacity;
    rented_block =
        reinterpret_cast<T*>(arena.allocate(sizeof(T) * reserve_capacity));
  }

  auto apply(const std::function<void(const T&)>& fn) const -> void {
    for (uint32_t i = 0; i < size; i++) {
      fn(rented_block[i]);
    }
  }

  constexpr auto insert(const T& data) -> void {
    if (size == capacity)
      grow();

    // Construct using the copy constructor.
    new (rented_block + (size++)) T(data);
  }

  constexpr auto contains(const T& data) const -> bool {
    for (uint32_t i = 0; i < size; i++) {
      if (rented_block[i] == data) {
        return true;
      }
    }

    return false;
  }

  constexpr auto at(uint32_t index) const -> T& { return rented_block[index]; }
  constexpr auto operator[](uint32_t index) -> T& { return at(index); }

  constexpr auto get_size() const -> uint32_t { return size; }
  constexpr auto get_arena() const -> Arena& { return arena; }
  constexpr auto get_view() const -> View::Vector<T> {
    return View::Vector<T>(rented_block, size);
  }

 private:
  auto grow() -> void {
    capacity *= growth_factor;
    auto new_block = reinterpret_cast<T*>(arena.allocate(sizeof(T) * capacity));

    std::memcpy(reinterpret_cast<void*>(new_block),
                reinterpret_cast<void*>(rented_block), sizeof(T) * size);
    rented_block = new_block;
  }

  Arena& arena;
  T* rented_block;
  uint32_t size;
  uint32_t capacity;
};

}  // namespace Perimortem::Memory::Managed
