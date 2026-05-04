// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data.hpp"
#include "perimortem/math/basic.hpp"

namespace Perimortem::Core::View {

// A raw read-only view of continuous bytes with no endianness.
//
// Byte views are typically used as strings but this is only valid when the
// encoding is ASCII. The main purpose of a byte view is to guarantee the data
// lie on 1 byte boundries (for instance ASCII is 7 bit aligned at the byte).
// Any optimized vectorization opperations will break UTF-8 strings unless you
// know exactly what you are doing (scanning for an exact byte).
//
// Since the structure of the data is undefined, Bytes data does not
// directly convert to Vector data, however Vector data can be converted
// to Bytes data.
class Bytes {
 public:
  using data_type = Byte;

  // Default to empty string.
  constexpr Bytes() : source_block(nullptr), size(0) {}

  constexpr Bytes(const View::Bytes&) = default;

  template <Count N>
  consteval Bytes(const Byte (&source)[N]) : source_block(&source), size(N) {}

  constexpr Bytes(const Byte* source, Count source_size)
      : source_block(source), size(source_size) {}

  constexpr auto operator==(const View::Bytes& rhs) const -> Bool {
    return rhs.size == size &&
           Data::compare(source_block, rhs.source_block, size);
  }

  constexpr auto operator!=(const View::Bytes& rhs) const -> Bool {
    return !operator==(rhs);
  }

  constexpr auto at(Count index) const -> const Byte {
    if (index > size) [[unlikely]] {
      return Byte();
    }

    return source_block[index];
  }

  constexpr auto operator[](Count index) const -> const Byte {
    return at(index);
  }

  constexpr auto slice(Count start, Count size) const -> View::Bytes {
    if (start >= get_size())
      return View::Bytes();

    return View::Bytes(source_block + start,
                       Math::min(size, get_size() - start));
  };

  constexpr auto operator[](Count index) -> Byte {
    if (index > size) {
      return Byte();
    }

    return source_block[index];
  };

  constexpr auto empty() const -> Bool { return size == 0; };
  constexpr auto get_size() const -> Count { return size; };
  constexpr auto get_data() const -> const Byte* { return source_block; };

 private:
  const Byte* source_block;
  Count size;
};
}  // namespace Perimortem::Core::View
