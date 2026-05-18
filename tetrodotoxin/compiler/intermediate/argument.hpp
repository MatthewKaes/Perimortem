// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

#include "tetrodotoxin/compiler/intermediate/type.hpp"

namespace Tetrodotoxin::Compiler::Intermediate {

// Represents a arg lowering from TTX to generic ISA
// TODO: This is a major hack and should all be moved out to an actual IR.
class Argument {
 public:
  Perimortem::Core::View::Bytes name;
  Type type;
};

}  // namespace Tetrodotoxin::Compiler::Intermediate
