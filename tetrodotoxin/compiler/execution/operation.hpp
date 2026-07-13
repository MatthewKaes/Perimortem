// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/static/union.hpp"

#include "tetrodotoxin/compiler/execution/binary.hpp"
#include "tetrodotoxin/compiler/execution/call.hpp"
#include "tetrodotoxin/compiler/execution/return.hpp"

namespace Tetrodotoxin::Compiler::Execution {

// One executable instruction in a Body.
using Operation = Perimortem::Core::Static::Union<Binary, Call, Return>;

}  // namespace Tetrodotoxin::Compiler::Execution
