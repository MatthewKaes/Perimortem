// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/access/amorphous.hpp"
#include "perimortem/math/hash.hpp"
#include "perimortem/core/view/amorphous.hpp"

namespace Perimortem::Memory::Static {

// Used to convert string literals into non-null terminated bytes.
template <Count literal_size>
class Bytes {
 public:
  constexpr Bytes() {}

  // Used for passing bytes directly that are already packed.
  constexpr Bytes(const Byte (&source)[literal_size]) {
    if consteval {
      for (Count i = 0; i < literal_size; i++) {
        content[i] = source[i];
      }
    } else {
      memcpy(content, source, literal_size);
    }
  }

  constexpr Bytes(const Core::View::Amorphous& source) {
    if consteval {
      for (Count i = 0; i < literal_size; i++) {
        content[i] = source.get_data()[i];
      }
    } else {
      memcpy(content, source.get_data(), literal_size);
    }
  }

  constexpr auto operator[](Count index) -> Byte& { return content[index]; }

  constexpr auto operator[](Count index) const -> const Byte& {
    return content[index];
  }

  template <Count slice_size>
  constexpr auto slice(Count start) const -> Bytes<slice_size> {
    Bytes<slice_size> chunk;
    if (start + slice_size >= literal_size) [[unlikely]] {
      return chunk;
    }

    if consteval {
      for (Count i = 0; i < slice_size; i++) {
        chunk[i] = content[start + i];
      }
    } else {
      memcpy(chunk.content, content + start, slice_size);
    }

    return chunk;
  }

  consteval auto get_size() const -> Count { return literal_size; }
  consteval auto get_capacity() const -> Count { return literal_size; }
  consteval auto get_view() const -> const Core::View::Amorphous {
    return Core::View::Amorphous(content, literal_size);
  }
  constexpr auto get_data() const -> const Byte* { return content; }
  constexpr auto get_data() -> Byte* { return content; }
  constexpr auto get_access() -> Core::Access::Amorphous {
    return Core::Access::Amorphous(content, literal_size);
  }

  constexpr auto hash() const -> Bits_64 {
    return Math::Hash(get_view()).get_value();
  }

 private:
  Byte content[literal_size]{};
};

}  // namespace Perimortem::Memory::Static
