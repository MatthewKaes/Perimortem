// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/static/bytes.hpp"

namespace Perimortem::Serialization::Encoding {

template <Count N>
struct NullTerminated {
  Byte source_block[N - 1]{};
  static constexpr Count size = N - 1;

  constexpr NullTerminated(char const (&src)[N]) {
    for (Count i = 0; i < N - 1; i++)
      source_block[i] = src[i];
  }

  constexpr auto get_size() const -> Count { return size; };
  constexpr auto get_data() const -> const Byte* { return source_block; };

  consteval auto get_view() const -> const Core::View::Amorphous {
    return Core::View::Amorphous(get_data(), get_size());
  }
};
}  // namespace Perimortem::Serialization::Encoding

template <Perimortem::Serialization::Encoding::NullTerminated null>
consteval Perimortem::Memory::Static::Bytes<null.size> operator""_bytes() {
  return Perimortem::Memory::Static::Bytes<null.size>(null.get_view());
}