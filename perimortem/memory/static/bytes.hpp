// Perimortem Engine
// Copyright © Matt Kaes

#pragma once


#include "perimortem/core/standard_types.hpp"

namespace Perimortem::Memory::Static {
// Special class for packing null terminated string constants.
template<Count N>
struct NullTerminated
{
    Byte content[N - 1]{};
    static constexpr Count size = N - 1;
    
    constexpr NullTerminated(char const(&src)[N])
    {
      for (Count i = 0; i < N - 1; i++)
        content[i] = src[i];
    }
};

// Used to convert string literals into non-null terminated bytes.
template <Count literal_size>
class Bytes {
 public:
  struct Aggragate {
    Byte content[literal_size]{};
  };

  constexpr Bytes() {}

  // Used for passing bytes directly that are already packed.
  constexpr Bytes(Aggragate bytes) : data(bytes) {}

  // Used for passing bytes directly that are already packed.
  constexpr Bytes(NullTerminated<literal_size + 1> source) {
    if consteval {
      for (Count i = 0; i < literal_size; i++) {
        data.content[i] = source.content[i];
      }
    } else {
      memcpy(data.content, source.content, literal_size);
    }
  }

  // Used for packing null terminator strings packed into byte arrays.
  constexpr Bytes(const Byte (&source)[literal_size]) {
    if consteval {
      for (Count i = 0; i < literal_size; i++) {
        data.content[i] = source[i];
      }
    } else {
      memcpy(data.content, source, literal_size);
    }
  }

  constexpr auto operator[](Count index) -> Byte& {
    return data.content[index];
  }

  constexpr auto operator[](Count index) const -> const Byte& {
    return data.content[index];
  }

  template <Count slice_size>
  constexpr auto slice(Count start) const -> Bytes<slice_size> {
    Bytes<slice_size> chunk;
    if (start + slice_size >= literal_size) [[unlikely]] {
      return chunk;
    }

    if consteval {
      for (Count i = 0; i < slice_size; i++) {
        chunk[i] = data.content[start + i];
      }
    } else {
      memcpy(chunk.data.content, data.content + start, slice_size);
    }

    return chunk;
  }

  constexpr auto get_size() const -> Count { return literal_size; }
  constexpr auto get_data() const -> const Byte* {
    return data.content;
  }
  constexpr auto get_data() -> Byte* {
    return data.content;
  }

 private:
  Aggragate data;
};

}  // namespace Perimortem::Memory::Static

template <Perimortem::Memory::Static::NullTerminated view>
constexpr Perimortem::Memory::Static::Bytes<view.size> operator""_bytes() {
  return Perimortem::Memory::Static::Bytes<view.size>(view);
}
