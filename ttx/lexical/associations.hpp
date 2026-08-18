// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/vector.hpp"

#include "ttx/concept/reference.hpp"
#include "ttx/lexical/anchor.hpp"

namespace Ttx::Lexical {

// Associations is the durable authored source index produced while one source
// transaction constructs its semantic graph. Its borrowed identities share the
// transaction Arena with that graph and remain useful after the mutable Cursor
// has completed parsing, linking, and finalization.
class Associations {
 public:
  constexpr Associations(Perimortem::Memory::Allocator::Arena& arena)
      : associations(arena) {}
  Associations(const Associations&) = delete;

  // Records one authored Anchor for the exact semantic identity constructed
  // there. A synthetic identity without an authored Span contributes nothing.
  auto create(Anchor anchor, const Ttx::Concept::Abstract& semantic) -> void;

  // Selects the most precise authored identity at one source byte. A focused
  // Token wins over a containing Span, then the narrower range wins. Equal
  // ranges retain construction order.
  auto find_at(Count offset) const
      -> Perimortem::Core::Option<const Ttx::Concept::Abstract&>;

 private:
  struct Association {
    Anchor anchor;
    Ttx::Concept::Reference<const Ttx::Concept::Abstract> semantic;
  };

  Perimortem::Memory::Managed::Vector<Association> associations;
};

}  // namespace Ttx::Lexical
