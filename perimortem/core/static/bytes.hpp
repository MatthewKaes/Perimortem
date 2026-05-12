// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/access/bytes.hpp"
#include "perimortem/core/hash.hpp"

namespace Perimortem::Core::Static {

// Used to convert string literals into non-null terminated bytes.
template <Count literal_size>
class Bytes {
 public:
  constexpr Bytes() {}

  // Used for passing bytes directly that are already packed.
  constexpr Bytes(const Byte (&source)[literal_size]) {
    if consteval {
      for (Count i = 0; i < literal_size; i++) {
        source_block[i] = source[i];
      }
    } else {
      memcpy(source_block, source, literal_size);
    }
  }

  constexpr Bytes(const Core::View::Bytes& source) {
    if consteval {
      for (Count i = 0; i < literal_size; i++) {
        source_block[i] = source.get_data()[i];
      }
    } else {
      memcpy(source_block, source.get_data(), literal_size);
    }
  }

  constexpr operator Core::View::Bytes() const { return get_view(); }
  constexpr operator Core::Access::Bytes() { return get_access(); }

  constexpr auto operator[](Count index) -> Byte& {
    return source_block[index];
  }

  constexpr auto operator[](Count index) const -> const Byte& {
    return source_block[index];
  }

  constexpr auto slice(Count start, Count size) const -> Core::View::Bytes {
    if (start >= get_size()) {
      return View::Bytes();
    }

    return View::Bytes(
        source_block + start, Math::min(size, get_size() - start));
  }

  constexpr auto get_size() const -> Count { return literal_size; }
  constexpr auto get_capacity() const -> Count { return literal_size; }
  constexpr auto get_view() const -> const Core::View::Bytes {
    return Core::View::Bytes(source_block, literal_size);
  }
  constexpr auto get_data() const -> const Byte* { return source_block; }
  constexpr auto get_data() -> Byte* { return source_block; }
  constexpr auto get_access() -> Core::Access::Bytes {
    return Core::Access::Bytes(source_block, literal_size);
  }

  constexpr auto hash() const -> Bits_64 {
    return Core::Hash(get_view()).get_value();
  }

 private:
  Byte source_block[literal_size]{};
};

}  // namespace Perimortem::Core::Static
