// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "perimortem/utility/range.hpp"

#include "ttx/type.hpp"

namespace Tetrodotoxin::Compiler::Symbol {

// Function describes one generated host-code function.
//
// `range` points into the compiler's machine-code buffer, while `source` keeps
// the original TTX function fact available for header generation and ABI checks.
class Function {
 public:
  Perimortem::Core::View::Bytes name;
  Perimortem::Utility::Range range;
  Ttx::Type::Function source;
};

}  // namespace Tetrodotoxin::Compiler::Symbol
