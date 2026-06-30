// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/perimortem.hpp"

#include "tetrodotoxin/compiler/abi/type.hpp"

namespace Tetrodotoxin::Compiler::Abi {

// One host ABI argument after validation has selected a boundary type.
class Argument {
 public:
  Perimortem::Core::View::Bytes name;
  Type type;
};

}  // namespace Tetrodotoxin::Compiler::Abi
