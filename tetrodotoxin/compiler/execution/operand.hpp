// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/union.hpp"

#include "tetrodotoxin/compiler/execution/addressable.hpp"
#include "tetrodotoxin/compiler/execution/constant.hpp"

namespace Tetrodotoxin::Compiler::Execution {

// A compiler input is either inline Constant data or an Addressable into the
// enclosing Body. The union's null state represents failed construction.
using Operand = Perimortem::Core::Static::Union<Addressable, Constant>;

}  // namespace Tetrodotoxin::Compiler::Execution
