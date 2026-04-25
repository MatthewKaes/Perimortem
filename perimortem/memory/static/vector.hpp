// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/structured.hpp"

namespace Perimortem::Memory::Static {
// Typically used for small value type arrays.
template <typename value_type, Count literal_size>
class Vector {
 public:
  struct Aggregate {
    value_type content[literal_size]{};
  };

  static constexpr Count size = literal_size;
  value_type content[literal_size]{};

  constexpr Vector() {}

  constexpr Vector(value_type (&source)[size]) {
    for (Count i = 0; i < size; i++) {
      content[i] = source[i];
    }
  }

  constexpr operator Core::View::Structured<value_type>() const {
    return Core::View::Structured<value_type>(content, literal_size);
  }


  constexpr auto operator[](Count index) -> value_type& {
    return content[index];
  }

  constexpr auto operator[](Count index) const -> const value_type& {
    return content[index];
  }

  constexpr auto get_size() const -> Count { return literal_size; }
};

}  // namespace Perimortem::Memory::Static
