// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/view/bytes.hpp"

#include "ttx/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Isa {

// The source representation of TTX documentation.
// Documentation is a continuous block of comment lines at the current cursor.
class Documentation {
 public:
  static auto evaluate(Ttx::Lexical::Cursor& cursor) -> Ttx::Documentation;
};

}  // namespace Tetrodotoxin::Isa
