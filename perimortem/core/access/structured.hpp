// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

namespace Perimortem::Core::Access {

// A raw read/write view of bytes with no endianness.
template <typename form>
class Structured {
 public:
  // Default to empty string.
  constexpr Structured() : source_block(nullptr), size(0) {}

  constexpr Structured(const Structured&) = default;

  constexpr Structured(form* source, Count source_size)
      : source_block(source), size(source_size) {}

  inline constexpr auto empty() const -> Bool { return size == 0; };
  inline constexpr auto get_size() const -> Count { return size; };
  inline constexpr auto get_data() -> form* { return source_block; };

  constexpr auto at(Count index) -> type& {
    if (index > size) [[unlikely]] {
      static type oob;
      return oob;
    }

    return source_block[index];
  }

  constexpr auto operator[](Count index) -> type& {
    return at(index);
  }

  inline constexpr auto slice(Count start, Count size) const
      -> Access::Structured<type> {
    if (start >= get_size())
      return Access::Structured<type>();

    const auto size_cap = get_size() - start;
    return Access::Structured<type>(source_block + start,
                              size > size_cap ? size_cap : size);
  };

 private:
  form* source_block;
  Count size;
};

}  // namespace Perimortem::Core::Access
