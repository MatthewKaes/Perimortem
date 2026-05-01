// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

namespace Perimortem::Utility {

// A simple tuple used for representing an association between key and value.
template <typename key_type, typename value_type>
struct Pair {
  key_type key;
  value_type value;
};

}  // namespace Perimortem::Utility
