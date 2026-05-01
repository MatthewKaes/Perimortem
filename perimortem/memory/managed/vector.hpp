// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/math/math.hpp"
#include "perimortem/core/view/structured.hpp"
#include "perimortem/memory/allocator/arena.hpp"

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

  constexpr operator Core::View::Structured<value_type>() const {
    return Core::View::Structured<value_type>(rented_block, size);
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

  constexpr auto emplace(const value_type&& data) -> value_type& {
    ensure_capacity(size + 1);

    // Construct using the move constructor.
    return *new (rented_block + (size++)) value_type(data);
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
  constexpr auto get_capacity() const -> Count { return capacity; };
  constexpr auto get_arena() const -> Allocator::Arena& { return arena; }
  constexpr auto get_view() const -> Core::View::Structured<value_type> {
    return Core::View::Structured<value_type>(rented_block, size);
  }

 private:
  // Ensures there is _at least_ enough room for the requested number of
  // objects.
  auto ensure_capacity(Count required_size) -> void {
    // Check if we can already fit required buffer.
    if (required_size <= get_capacity()) {
      return;
    }

    // Attempt to grow by a factor of 2.
    // If that doesn't work than grow to exact size.
    const auto new_capacity =
        Math::max(get_capacity() * 2, required_size);

    // Fetch and transfer to new block.
    auto new_block = reinterpret_cast<value_type*>(
        arena.allocate(sizeof(value_type) * capacity));

    if (rented_block) {
      memcpy(new_block, rented_block, sizeof(value_type) * size);
    }

    // Update block and get the new capacity.
    rented_block = new_block;

    // Get the actual capacity provided which is often more than we actual
    // requested.
    capacity = new_block->get_usable_bytes() / sizeof(value_type);
  }

  auto grow() -> void {
    capacity *= growth_factor;
    auto new_block = reinterpret_cast<value_type*>(
        arena.allocate(sizeof(value_type) * capacity));

    Core::Data::copy(reinterpret_cast<Byte*>(new_block), rented_block,
                    sizeof(value_type) * size);
    rented_block = new_block;
  }

  Allocator::Arena& arena;
  value_type* rented_block;
  Count size;
  Count capacity;
};

}  // namespace Perimortem::Memory::Managed
