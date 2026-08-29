// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"

#include "ttx/bootstrap/concept/none.hpp"
#include "ttx/bootstrap/concept/unknown.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Static is one Composite's Type-level authority. Nested Types, static Fields,
// immutable declarations, and Static Callables share one spelling space, so a
// name always selects one exact semantic identity.
class Static : public Ttx::Concept::Abstract {
 public:
  constexpr explicit Static(Perimortem::Memory::Allocator::Arena& arena)
      : bindings(arena) {}

  TTX_CONTRACT(Static, Ttx::Concept::Abstract);
  TTX_NAME("static"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto can_bind(const Ttx::Concept::Abstract& binding) const -> Bool;
  auto bind(Ttx::Concept::Abstract& binding, Bool published) -> Bool;
  auto is_published(const Ttx::Concept::Abstract& binding) const -> Bool;
  auto resolve_published(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract&;
  auto resolve_concept(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract& override;
  auto visit_concepts(ttx_named_abstract_callable* visitor) const
      -> void override;

  constexpr auto complete() -> void { completed = True; }

 private:
  class Binding {
   public:
    constexpr Binding(Ttx::Concept::Abstract& semantic, Bool published)
        : semantic(&semantic), published(published) {}

    Ttx::Concept::Abstract* semantic;
    Bool published;
  };

  Perimortem::Memory::Managed::Map<Perimortem::Core::View::Bytes, Binding>
      bindings;
  Bool completed = False;
};

}  // namespace Tetrodotoxin::Library::Language::Types
