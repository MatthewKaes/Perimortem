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

constexpr auto log2(Bits_64 value) -> Bits_64 {
  return 64 - __builtin_clzg(value, Signed_32(sizeof(Bits_64) * 8));
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
