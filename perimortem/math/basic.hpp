// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/perimortem.hpp"

#pragma once

namespace Perimortem::Math {

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

constexpr auto log(Real_64 value) -> Real_64 {
  return __builtin_log(value);
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

}  // namespace Perimortem::Math
