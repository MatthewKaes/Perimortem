// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data_model.hpp"
#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/view/vector.hpp"

namespace Perimortem::Memory::Managed {

// A simple linear flat array of trivially constructable values.
//
template <typename value_type>
class Vector {
 public:
  static constexpr Count start_capacity = 8;
  static constexpr Count growth_factor = 2;

  Vector(const Vector&) = default;
  Vector(Allocator::Arena& arena) : arena(arena) { reset(); }

  constexpr operator View::Vector<value_type>() const {
    return View::Vector<value_type>(rented_block, size);
  }

  auto clear() -> void { size = 0; }

  auto reset() -> void {
    size = 0;
    capacity = start_capacity;
    rented_block = reinterpret_cast<value_type*>(
        arena.allocate(sizeof(value_type) * start_capacity));
  }

  auto reset(Count reserve_capacity) -> void {
    if (reserve_capacity <= start_capacity) {
      reserve_capacity = start_capacity;
    }

    size = 0;
    capacity = reserve_capacity;
    rented_block = reinterpret_cast<value_type*>(
        arena.allocate(sizeof(value_type) * reserve_capacity));
  }

  constexpr auto insert(const value_type& data) -> void {
    if (size == capacity)
      grow();

    // Construct using the copy constructor.
    new (rented_block + (size++)) value_type(data);
  }

  constexpr auto contains(const value_type& data) const -> Bool {
    for (Count i = 0; i < size; i++) {
      if (rented_block[i] == data) {
        return true;
      }
    }

    return false;
  }

  constexpr auto at(Count index) const -> value_type& {
    return rented_block[index];
  }
  constexpr auto operator[](Count index) -> value_type& { return at(index); }

  constexpr auto get_size() const -> Count { return size; }
  constexpr auto get_arena() const -> Allocator::Arena& { return arena; }
  constexpr auto get_view() const -> View::Vector<value_type> {
    return View::Vector<value_type>(rented_block, size);
  }

 private:
  auto grow() -> void {
    capacity *= growth_factor;
    auto new_block = reinterpret_cast<value_type*>(
        arena.allocate(sizeof(value_type) * capacity));

    Core::copy(reinterpret_cast<void*>(new_block), rented_block,
               sizeof(value_type) * size);
    rented_block = new_block;
  }

  Allocator::Arena& arena;
  value_type* rented_block;
  Count size;
  Count capacity;
};

}  // namespace Perimortem::Memory::Managed
