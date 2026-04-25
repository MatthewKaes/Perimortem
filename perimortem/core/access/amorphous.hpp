// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Core::Access {

// A raw read/write view of bytes with no endianness.
class Amorphous {
 public:
  // Default to empty string.
  constexpr Amorphous() : source_block(nullptr), size(0) {}

  constexpr Amorphous(const Amorphous&) = default;

  constexpr Amorphous(Byte* source, Count source_size)
      : source_block(source), size(source_size) {}

  inline constexpr auto empty() const -> Bool { return size == 0; };
  inline constexpr auto get_size() const -> Count { return size; };
  inline constexpr auto get_data() -> Byte* { return source_block; };

  constexpr auto at(Count index) -> Byte& {
    if (index > size) [[unlikely]] {
      static Byte oob;
      return oob;
    }

    return source_block[index];
  }

  constexpr auto operator[](Count index) -> Byte& {
    return at(index);
  }

  inline constexpr auto slice(Count start, Count size) const
      -> Access::Amorphous {
    if (start >= get_size())
      return Access::Amorphous();

    const auto size_cap = get_size() - start;
    return Access::Amorphous(source_block + start,
                             size > size_cap ? size_cap : size);
  };

 private:
  Byte* source_block;
  Count size;
};

}  // namespace Perimortem::Core::Access
