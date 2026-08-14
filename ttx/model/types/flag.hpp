// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/types/value.hpp"

namespace Ttx::Model::Types {

// Flag is the Terminal contract for a binary logical domain. Concrete logical
// Types supply their fixed name, representation, and documentation.
class Flag : public Value {
 public:
  TTX_CONTRACT(Flag, Value);
};

}  // namespace Ttx::Model::Types
