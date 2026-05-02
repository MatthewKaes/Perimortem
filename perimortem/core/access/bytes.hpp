// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/math/basic.hpp"

namespace Perimortem::Core::Access {

// A raw read/write view of bytes with no endianness.
class Bytes {
 public:
  using data_type = Byte;

  // Default to empty string.
  constexpr Bytes() : source_block(nullptr), size(0) {}

  constexpr Bytes(const Bytes&) = default;

  constexpr Bytes(data_type* source, Count source_size)
      : source_block(source), size(source_size) {}

  template <Count N>
  constexpr Bytes(data_type (&source)[N]) : source_block(source), size(N) {}

  constexpr auto at(Count index) -> data_type& {
    if (index > size) [[unlikely]] {
      static data_type oob;
      return oob;
    }

    return source_block[index];
  }

  constexpr auto operator[](Count index) -> data_type& { return at(index); }

  constexpr auto slice(Count start, Count size) const -> Access::Bytes {
    if (start >= get_size())
      return Access::Bytes();

    return Access::Bytes(source_block + start,
                         Math::min(size, get_size() - start));
  };

  constexpr auto empty() const -> Bool { return size == 0; };
  constexpr auto get_size() const -> Count { return size; };
  constexpr auto get_data() -> data_type* { return source_block; };

 private:
  data_type* source_block;
  Count size;
};

}  // namespace Perimortem::Core::Access
