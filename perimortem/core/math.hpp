// Perimortem Engine
// Copyright © Matt Kaes

#include "perimortem/core/standard_types.hpp"

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

}
