// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "perimortem/utility/option.hpp"

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/layout.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Parser {

// Layout consumes one complete Library signature shape and constructs its real
// TTX Layout and semantic edges in the supplied graph Arena.
class Layout {
 public:
  Layout() = delete;

  static auto parse(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& context)
      -> Perimortem::Utility::Option<const Ttx::Concept::Layout&>;
};

}  // namespace Tetrodotoxin::Library::Language::Parser
