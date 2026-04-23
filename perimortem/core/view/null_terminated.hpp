// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"
#include "perimortem/core/view/amorphous.hpp"

namespace Perimortem::Core::View {

template <Count N>
struct NullTerminated {
  Byte source_block[N - 1]{};
  static constexpr Count size = N - 1;

  constexpr NullTerminated(char const (&src)[N]) {
    for (Count i = 0; i < N - 1; i++)
      source_block[i] = src[i];
  }

  inline constexpr auto get_size() const -> Count { return N; };
  inline constexpr auto get_data() const -> const Byte* {
    return source_block;
  };
};
}  // namespace Perimortem::Utility

template <Perimortem::Core::View::NullTerminated null>
constexpr Perimortem::Core::View::Amorphous operator""_view() {
  return Perimortem::Core::View::Amorphous(null.get_data(), null.get_size());
}