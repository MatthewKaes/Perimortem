// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/types/value.hpp"

namespace Ttx::Model::Types {

// Real is the Terminal contract for an IEEE floating-point domain. Concrete
// precision Types supply their fixed name, representation, and documentation.
class Real : public Value {
 public:
  TTX_CONTRACT(Real, Value);
};

}  // namespace Ttx::Model::Types
