// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"
#include "perimortem/core/static/union.hpp"

namespace Tetrodotoxin::Compiler::Execution {

// The complete set of inline values accepted by compiler operations. The
// active alternative is also the value's lowering type.
using Constant = Perimortem::Core::Static::
    Union<Unsigned_64, Signed_64, Real_64, Perimortem::Core::View::Bytes, Bool>;

}  // namespace Tetrodotoxin::Compiler::Execution
