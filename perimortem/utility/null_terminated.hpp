// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/bytes.hpp"
#include "perimortem/core/view/bytes.hpp"

namespace Perimortem::Utility {
template <CppSize size_including_null>
struct NullTerminated {
  Byte content[size_including_null - 1]{};

  consteval NullTerminated(SignedBits_8 const (&source)[size_including_null]) {
    for (Count i = 0; i < get_size(); i++) {
      content[i] = source[i];
    }
  }

  consteval operator Core::View::Bytes() const { return get_view(); }

  consteval auto get_size() const -> Count { return size_including_null - 1; }
  consteval auto get_data() const -> const Byte* { return content; }
  consteval auto get_view() const -> const Core::View::Bytes {
    return Core::View::Bytes(content, get_size());
  }
};
}  // namespace Perimortem::Utility

// Converts C++ string literals into valid Perimortem byte strings.
template <Perimortem::Utility::NullTerminated c_string>
consteval const Perimortem::Core::View::Bytes operator""_view( ) {
  return c_string.get_view();
}

template <Perimortem::Utility::NullTerminated c_string>
consteval Perimortem::Core::Static::Bytes<c_string.get_size()>
operator""_bytes() {
  return Perimortem::Core::Static::Bytes<c_string.get_size()>(
      c_string.get_view());
}