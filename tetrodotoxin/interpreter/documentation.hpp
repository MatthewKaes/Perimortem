// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "ttx/concept/documentation.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Interpreter {

// Documentation evaluates the comment prefix shared by authored definitions.
// It constructs the first class TTX Documentation value directly from source
// order without creating a private syntax record for each Dialect.
class Documentation {
 public:
  static auto evaluate(Ttx::Lexical::Cursor& cursor)
      -> const Ttx::Concept::Documentation&;
};

}  // namespace Tetrodotoxin::Interpreter
