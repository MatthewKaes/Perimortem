// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/option.hpp"

#include "tetrodotoxin/library/language/constant.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "ttx/concept/abstract.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Parser {

// Literal consumes one complete scalar or byte operand and constructs its real
// Constant in the graph Arena. Library's binary wide scalar addresses supply
// inferred Types while the source context remains the sole owner of Embedded
// resolution. Contextual fitting begins only after the complete Expression has
// synthesized its Type, so Literal accepts no target Type. Embedded Constants
// borrow Resource bytes, requiring the supplied domain not to outlive the
// Resource dependency domain.
class Literal {
 public:
  Literal() = delete;

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Materializations& materializations,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& source_context)
      -> Perimortem::Utility::Option<Constant&>;
};

}  // namespace Tetrodotoxin::Library::Language::Parser
