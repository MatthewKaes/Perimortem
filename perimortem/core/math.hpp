// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/perimortem.hpp"

#pragma once

namespace Perimortem::Core::Math {

template <typename type>
constexpr auto max(type left, type right) -> type {
  return left > right ? left : right;
}

template <typename type>
constexpr auto min(type left, type right) -> type {
  return left < right ? left : right;
}

template <typename type>
constexpr auto clamp(type value, type min_value, type max_value) -> type {
  return max(min(value, max_value), min_value);
}

template <typename type>
constexpr auto wrap(type value, type modulo) -> type {
  value %= modulo;
  return value < 0 ? value + modulo : value;
}

template <typename type>
constexpr auto absolute(type value) -> type {
  return value >= 0 ? value : -value;
}

// Full width values skip the terminal shift because C++ does not define a
// shift by the size of the value. Smaller widths compare against the first
// excluded value which avoids manufacturing host minimum and maximum values.
constexpr auto is_representable(Signed_64 value, Count byte_width) -> Bool {
  if (byte_width == 0 || byte_width > sizeof(Signed_64)) {
    return False;
  }

  if (byte_width == sizeof(Signed_64)) {
    return True;
  }

  Signed_64 limit = Signed_64(1) << (byte_width * 8 - 1);
  return value >= -limit && value < limit;
}

constexpr auto is_representable(Unsigned_64 value, Count byte_width) -> Bool {
  if (byte_width == 0 || byte_width > sizeof(Unsigned_64)) {
    return False;
  }

  if (byte_width == sizeof(Unsigned_64)) {
    return True;
  }

  Unsigned_64 limit = Unsigned_64(1) << (byte_width * 8);
  return value < limit;
}

constexpr auto log2(Unsigned_64 value) -> Unsigned_64 {
  return 64 - __builtin_clzg(value, Signed_32(sizeof(Unsigned_64) * 8));
}

constexpr auto sqrt(Real_64 value) -> Real_64 {
  return __builtin_sqrt(value);
}

constexpr auto floor(Real_64 value) -> Count {
  return Count(__builtin_floor(value));
}

constexpr auto ceil(Real_64 value) -> Count {
  return Count(__builtin_ceil(value));
}

}  // namespace Perimortem::Core::Math
