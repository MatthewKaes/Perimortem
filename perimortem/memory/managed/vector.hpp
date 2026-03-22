// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/dynamic/record.hpp"
#include "perimortem/memory/const/standard_types.hpp"
#include "perimortem/memory/view/vector.hpp"

#include <cstring>

namespace Perimortem::Memory::Managed {

// A simple linear flat array of trivially constructable values.
template <typename T>
class Vector {
 public:
  static constexpr Count start_capacity = 8;
  static constexpr Count growth_factor = 2;

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

  auto reset(Count reserve_capacity) -> void {
    if (reserve_capacity <= start_capacity) {
      reserve_capacity = start_capacity;
    }

    size = 0;
    capacity = reserve_capacity;
    rented_block =
        reinterpret_cast<T*>(arena.allocate(sizeof(T) * reserve_capacity));
  }

  auto apply(const std::function<void(const T&)>& fn) const -> void {
    for (Count i = 0; i < size; i++) {
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
    for (Count i = 0; i < size; i++) {
      if (rented_block[i] == data) {
        return true;
      }
    }

    return false;
  }

  constexpr auto at(Count index) const -> T& { return rented_block[index]; }
  constexpr auto operator[](Count index) -> T& { return at(index); }

  constexpr auto get_size() const -> Count { return size; }
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
  Count size;
  Count capacity;
};

}  // namespace Perimortem::Memory::Managed
