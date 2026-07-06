// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

namespace Tetrodotoxin::Compiler::Symbol {

// External names a foreign symbol that generated machine code calls but does
// not define. The linker turns these records into undefined object symbols.
class External {
 public:
  Perimortem::Core::View::Bytes name;
};

}  // namespace Tetrodotoxin::Compiler::Symbol
