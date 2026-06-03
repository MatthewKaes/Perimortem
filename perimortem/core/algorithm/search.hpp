// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/math.hpp"

namespace Perimortem::Core::Algorithm {

// Fast vectorized sub string search for View::Bytes
auto search(View::Bytes src, View::Bytes value) -> Count;

// Returns the index of the smallest element in a View::Vector.
// If multiple elements are the smallest then the lowest index is used.
template <typename element_type>
constexpr auto min_element(View::Vector<element_type> src) -> Count {
  Count target_index = 0;

  for (Count i = 1; i < src.get_size(); i++) {
    if (src[i] < src[target_index]) {
      target_index = i;
    }
  }

  return target_index;
}

// Returns the index of the largest element in a View::Vector.
// If multiple elements are the largest then the lowest index is used.
template <typename element_type>
constexpr auto max_element(View::Vector<element_type> src) -> Count {
  Count target_index = 0;

  for (Count i = 1; i < src.get_size(); i++) {
    if (src[i] > src[target_index]) {
      target_index = i;
    }
  }

  return target_index;
}

}  // namespace Perimortem::Core::Algorithm
