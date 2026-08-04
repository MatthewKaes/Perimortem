// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/library/language/expression.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Parser {

// Expression consumes one complete Library value operand. It selects the
// primary parser and dispatches each following operator in precedence order.
// The concrete operator owns its complete grammar, construction, diagnostics,
// and eager folding transaction. Receiving Type fitting begins only after the
// complete Expression returns.
class Expression {
 public:
  Expression() = delete;

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context)
      -> Perimortem::Utility::Option<const Language::Expression&>;
};

}  // namespace Tetrodotoxin::Library::Language::Parser
