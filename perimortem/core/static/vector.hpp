// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/access/vector.hpp"
#include "perimortem/core/view/vector.hpp"

namespace Perimortem::Core::Static {
// Typically used for small value type arrays.
template <typename type, Count literal_size>
class Vector {
 public:
  static constexpr Count size = literal_size;

  constexpr Vector() {}

  constexpr Vector(type (&source)[size]) {
    for (Count i = 0; i < size; i++) {
      source_block[i] = source[i];
    }
  }

  constexpr operator Core::View::Vector<type>() const { return get_view(); }
  constexpr operator Core::Access::Vector<type>() { return get_access(); }

  constexpr auto operator[](Count index) -> type& { return source_block[index]; }

  constexpr auto operator[](Count index) const -> const type& {
    return source_block[index];
  }

  consteval auto get_size() const -> Count { return literal_size; }
  consteval auto get_capacity() const -> Count { return literal_size; }
  consteval auto get_view() const -> const Core::View::Vector<type> {
    return Core::View::Vector<type>(source_block, literal_size);
  }
  constexpr auto get_data() const -> const type* { return source_block; }
  constexpr auto get_data() -> type* { return source_block; }
  constexpr auto get_access() -> Core::Access::Vector<type> {
    return Core::Access::Vector<type>(source_block, literal_size);
  }

 private:
  type source_block[literal_size]{};
};

}  // namespace Perimortem::Core::Static
