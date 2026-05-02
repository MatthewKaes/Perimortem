// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/bytes.hpp"

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

  consteval auto get_view() const -> const Core::View::Bytes {
    return Core::View::Bytes(get_data(), get_size());
  }
};
}  // namespace Perimortem::Serialization::Encoding

template <Perimortem::Serialization::Encoding::NullTerminated null>
consteval Perimortem::Core::Static::Bytes<null.size> operator""_bytes() {
  return Perimortem::Core::Static::Bytes<null.size>(null.get_view());
}