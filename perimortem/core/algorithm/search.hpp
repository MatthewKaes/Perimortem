// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/data.hpp"
#include "perimortem/core/math.hpp"

namespace Perimortem::Core::Algorithm {

// Fast vectorized sub string search for View::Bytes
auto search(View::Bytes src, View::Bytes value) -> Count;

}  // namespace Perimortem::Core::Algorithm
