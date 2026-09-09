// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.hpp"
#include "ttx/concept/bound.hpp"
#include "ttx/concept/visitor.hpp"

namespace Ttx::Concept {

// Scope exposes the names an owner makes reachable at this boundary. The
// returned edges retain their own policies. Asking a selected edge about an
// Import therefore happens before another lookup or resolution crosses it.
class Scope {
 public:
  using Visitor = Concept::Visitor<Abstract::Handle>;

  struct Operations {
    auto (*resolve_concept)(const void*, Perimortem::Core::View::Bytes)
        -> Abstract::Handle;
    auto (*visit_concepts)(const void*, Visitor) -> void;
  };

  class Handle : public Bound<Operations> {
   public:
    using Bound::Bound;

    auto resolve_concept(Perimortem::Core::View::Bytes name) const
        -> Abstract::Handle {
      return operations.resolve_concept(source, name);
    }

    // Enumeration has the native discovery lifetime: the receiver is borrowed
    // for the call and names can be temporary. Bound values retain the lifetime
    // of their provider state and code, with no conversion to a native class.
    auto visit_concepts(Visitor visitor) const -> void {
      operations.visit_concepts(source, visitor);
    }
  };
};

}  // namespace Ttx::Concept
