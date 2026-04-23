// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/access/amorphous.hpp"
#include "perimortem/core/view/amorphous.hpp"
#include "perimortem/core/view/null_terminated.hpp"

namespace Perimortem::Memory::Static {

// Used to convert string literals into non-null terminated bytes.
template <Count literal_size>
class Bytes {
 public:
  struct Aggregate {
    Byte content[literal_size]{};
  };

  constexpr Bytes() {}

  // Used for passing bytes directly that are already packed.
  constexpr Bytes(Aggregate bytes) : data(bytes) {}

  // Used for passing bytes directly that are already packed.
  constexpr Bytes(Core::View::NullTerminated<literal_size + 1> source) {
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
  constexpr auto get_capacity() const -> Count { return literal_size; }
  constexpr auto get_view() const -> const Core::View::Amorphous {
    return Core::View::Amorphous(data.content, literal_size);
  }
  constexpr auto get_access() -> Core::Access::Amorphous {
    return Core::Access::Amorphous(data.content, literal_size);
  }

 private:
  Aggregate data;
};

}  // namespace Perimortem::Memory::Static

template <Perimortem::Core::View::NullTerminated view>
constexpr Perimortem::Memory::Static::Bytes<view.size> operator""_bytes() {
  return Perimortem::Memory::Static::Bytes<view.size>(view);
}
