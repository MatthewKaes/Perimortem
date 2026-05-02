// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/access/bytes.hpp"
#include "perimortem/math/basic.hpp"

namespace Perimortem::Core::Access {

// A read-write view of continuous data with possible endianness and structure.
//
// Vector data can be converted to Bytes data in order to interperet it
// at a byte level, however this is only valid in memory. To write and read
// binary data as vector data it should be serialized using
// `Perimortem::Serialization::Binary`.
template <typename type>
class Vector {
 public:
  using data_type = type;

  // Default to empty string.
  constexpr Vector() = default;
  constexpr Vector(const Vector&) = default;

  constexpr Vector(data_type* source, Count source_size)
      : source_block(source), size(source_size) {}

  template <Count N>
  constexpr Vector(data_type (&source)[N]) : source_block(source), size(N) {}

  constexpr auto at(Count index) -> data_type& {
    if (index > size) [[unlikely]] {
      static data_type oob;
      return oob;
    }

    return source_block[index];
  }

  constexpr auto operator[](Count index) -> data_type& { return at(index); }

  constexpr auto slice(Count start, Count size) const
      -> Access::Vector<data_type> {
    if (start >= get_size())
      return Access::Vector<data_type>();

    return Access::Vector<data_type>(source_block + start,
                                     Math::min(size, get_size() - start));
  };

  constexpr auto empty() const -> Bool { return size == 0; };
  constexpr auto get_size() const -> Count { return size; };
  constexpr auto get_data() -> data_type* { return source_block; };
  constexpr auto get_bytes() const -> Bytes {
    return Bytes(source_block, size * sizeof(data_type));
  }

 private:
  type* source_block = nullptr;
  Count size = 0;
};

}  // namespace Perimortem::Core::Access
