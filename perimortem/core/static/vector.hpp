// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/access/vector.hpp"

namespace Perimortem::Core::Static {
// Typically used for small value type arrays.
template <typename type, Count literal_size>
class Vector {
 public:
  static constexpr Count size = literal_size;

  constexpr Vector() {}

  // Allows direct value initialization: Static::Vector<Bits_8, 3>{1, 2, 3}.
  template <typename... value_pack>
    requires(sizeof...(value_pack) == size)
  constexpr Vector(value_pack... values) : source_block{type(values)...} {}

  constexpr Vector(type (&source)[size]) {
    for (Count i = 0; i < size; i++) {
      source_block[i] = source[i];
    }
  }

  constexpr Vector(const type (&source)[size]) {
    for (Count i = 0; i < size; i++) {
      source_block[i] = source[i];
    }
  }

  constexpr Vector(type (*generator)(Count)) {
    for (Count i = 0; i < literal_size; i++) {
      source_block[i] = generator(i);
    }
  }

  // Fast read function that assumes the read range is valid.
  static constexpr auto read_range(const type* source) {
    Vector data;
    if consteval {
      for (Count i = 0; i < literal_size; i++) {
        data.source_block[i] = source[i];
      }
    } else {
      if constexpr (literal_size < 16) {
        for (Count i = 0; i < literal_size; i++) {
          data.source_block[i] = source[i];
        }
      } else {
        Data::copy(data.source_block, source, literal_size);
      }
    }

    return data;
  }

  constexpr operator Core::View::Vector<type>() const { return get_view(); }
  constexpr operator Core::Access::Vector<type>() { return get_access(); }

  constexpr auto operator==(const View::Vector<type>& rhs) -> Bool {
    if (rhs.get_size() != literal_size) {
      return False;
    }

    return Data::compare(source_block, rhs.get_data(), literal_size);
  }

  constexpr auto operator!=(const View::Vector<type>& rhs) -> Bool {
    return !(*this == rhs);
  }

  constexpr auto operator[](Count index) -> type& {
    return source_block[index];
  }

  constexpr auto operator[](Count index) const -> const type& {
    return source_block[index];
  }

  constexpr auto slice(Count start, Count size = Count(-1)) const
      -> Core::View::Vector<type> {
    return get_view().slice(start, size);
  }

  constexpr auto is_empty() const -> Bool { return literal_size == 0; };
  constexpr auto get_size() const -> Count { return literal_size; }
  constexpr auto get_capacity() const -> Count { return literal_size; }
  constexpr auto get_view() const -> const Core::View::Vector<type> {
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
