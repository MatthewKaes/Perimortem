// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/model/types/value.hpp"

namespace Ttx::Model::Types {

// Signed is the Terminal contract for a value that can store both positive and
// negative integer values.
class Signed : public Value {
 public:
  TTX_CONTRACT(Signed, Value, 0xd6027af70ac64c12, 0x8a76a2ead91b2550);
};

}  // namespace Ttx::Model::Types
