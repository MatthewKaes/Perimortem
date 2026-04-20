// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/data_model.hpp"
#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/static/bytes.hpp"

namespace Perimortem::Memory::View {

// A raw read-only view of bytes with no endianness.
//
// Byte views are typically used as strings but this is only valid when the
// encoding is ASCII. The main purpose of a byte view is to guarantee the data
// lie on 1 byte boundries (for instance ASCII is 7 bit aligned at the byte).
// Any optimized vectorization opperations will break UTF-8 strings unless you
// know exactly what you are doing (scanning for an exact byte).
class Bytes {
 public:
  // Default to empty string.
  constexpr Bytes() : source_block(nullptr), size(0) {}

  constexpr Bytes(const Bytes&) = default;

  template <Count N>
  constexpr Bytes(const Byte (&source)[N]) : source_block(&source), size(N) {}

  template <Count N>
  constexpr Bytes(const Static::Bytes<N> source)
      : source_block(source.content), size(N) {}

  constexpr Bytes(const Byte* source, Count source_size)
      : source_block(source), size(source_size) {}

  inline constexpr auto operator==(const Bytes& rhs) const -> Bool {
    if consteval {
      if (rhs.get_size() != size) {
        return false;
      }

      for (Count i = 0; i < size; i++) {
        if (rhs.get_data()[i] != get_data()[i]) {
          return false;
        }
      }

      return true;
    } else {
      return rhs.size == size &&
             Core::compare(source_block, rhs.source_block, size) == 0;
    }
  }

  inline constexpr auto empty() const -> Bool { return size == 0; };
  inline constexpr auto get_size() const -> Count { return size; };
  inline constexpr auto get_data() const -> const Byte* {
    return source_block;
  };

  inline constexpr auto slice(Count start, Count size) const -> Bytes {
    if (start >= get_size())
      return View::Bytes();

    return View::Bytes(source_block + start,
                       Core::Math::min(size, get_size() - start));
  };

  inline constexpr auto operator[](Count index) const -> Byte {
    return source_block[index];
  };

  // Scans a 32 bytes block for the offset of a Byteacter.
  //
  // WARNING: Provides no bounds protection, use only when it's known the block
  // is valid.
  auto fast_scan(Byte search, Count position = 0) const -> Count;

  // Scans the source array for the position of a given byte starting at the
  // specified position.
  // Returns "-1" if no instances can be found.
  auto scan(Byte search, Count position = 0) const -> Count;

 private:
  const Byte* source_block;
  Count size;
};

}  // namespace Perimortem::Memory::View

template <Perimortem::Memory::Static::NullTerminated view>
constexpr Perimortem::Memory::View::Bytes operator""_view() {
  return Perimortem::Memory::View::Bytes(view.content, view.size);
}
