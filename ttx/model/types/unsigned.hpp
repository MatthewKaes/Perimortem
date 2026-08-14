// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/types/value.hpp"

namespace Ttx::Model::Types {

// Signed is the Terminal contract for a value that can store both positive and
// negative integer values.
class Unsigned : public Value {
 public:
  TTX_CONTRACT(Unsigned, Value);
};

}  // namespace Ttx::Model::Types
