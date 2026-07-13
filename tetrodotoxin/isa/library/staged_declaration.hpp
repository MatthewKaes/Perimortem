// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/utility/range.hpp"

#include "tetrodotoxin/isa/base/declaration.hpp"
#include "ttx/type.hpp"

namespace Tetrodotoxin::Isa::Library {

// A Library declaration retained until its type is first referenced.
class StagedDeclaration {
 public:
  enum class State : Bits_8 {
    Pending,
    Evaluating,
    Ready,
    Failed,
  };

  Tetrodotoxin::Isa::Base::Declaration declaration;
  Perimortem::Utility::Range source;
  const Ttx::Type* type = nullptr;
  State state = State::Pending;
};

}  // namespace Tetrodotoxin::Isa::Library
