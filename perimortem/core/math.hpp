// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/perimortem.hpp"

#pragma once

namespace Perimortem::Core::Math {

template <typename type>
constexpr auto max(type a, type b) -> type {
  return a > b ? a : b;
}

template <typename type>
constexpr auto min(type a, type b) -> type {
  return a < b ? a : b;
}

template <typename type>
constexpr auto clamp(type val, type min_val, type max_val) -> type {
  return max(min(val, max_val), min_val);
}

template <typename type>
constexpr auto wrap(type val, type modulo) -> type {
  val %= modulo;
  return val < 0 ? val + modulo : val;
}

template <typename type>
constexpr auto absolute(type a) -> type {
  return a >= 0 ? a : -a;
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
