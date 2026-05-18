// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/perimortem.hpp"

#include "tetrodotoxin/compiler/intermediate/argument.hpp"

namespace Tetrodotoxin::Compiler::Context {

class Function {
 public:
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::View::Vector<Intermediate::Argument> args;
  Intermediate::Type result;
  Count symbol;
};

}  // namespace Tetrodotoxin::Compiler::Context
