// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/view/vector.hpp"
#include "perimortem/core/perimortem.hpp"

#include "tetrodotoxin/compiler/abi/argument.hpp"

namespace Tetrodotoxin::Compiler::Context {

class Function {
 public:
  Perimortem::Core::View::Bytes name;
  Perimortem::Core::View::Vector<Abi::Argument> arguments;
  Abi::Type return_type;
  Count symbol_index;
};

}  // namespace Tetrodotoxin::Compiler::Context
