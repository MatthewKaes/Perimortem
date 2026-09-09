// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"

#include "ttx/concept/none.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/concept/unknown.hpp"

namespace Tetrodotoxin::Library::Language::Access {

// Access indexing is split for composite structures depending on the reciever.
// Instance provides access to any abstracts associated with data flows:
//
// * Any Fields that take up space for an allocated instance of the Type.
// * Any Callables that take [self] as a parameter.
//
// Instance access then follow whenever you have `addressable (access op) X`.
class Instance : public Ttx::Concept::Abstract {
 public:
  constexpr explicit Instance(Perimortem::Memory::Allocator::Arena& arena)
      : bindings(arena) {}

  TTX_CONTRACT(Instance, Ttx::Concept::Abstract);
  TTX_NAME("instance"_view);
  TTX_EMPTY_DOCUMENTATION();

  auto can_bind(const Ttx::Concept::Abstract& binding) const -> Bool;
  auto bind(Ttx::Concept::Abstract& binding, Bool published) -> Bool;
  auto is_published(const Ttx::Concept::Abstract& binding) const -> Bool;
  auto resolve_published(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract&;
  auto resolve_concept(Perimortem::Core::View::Bytes name) const
      -> const Ttx::Concept::Abstract& override;
  auto visit_concepts(Ttx::Concept::Abstract::Visitor visitor) const
      -> void override;

  constexpr auto complete() -> void { completed = True; }

 private:
  class Binding {
   public:
    constexpr Binding(Ttx::Concept::Abstract& semantic, Bool published)
        : semantic(semantic), published(published) {}

    Ttx::Concept::Reference<Ttx::Concept::Abstract> semantic;
    Bool published;
  };

  Perimortem::Memory::Managed::Map<Perimortem::Core::View::Bytes, Binding>
      bindings;
  Bool completed = False;
};

}  // namespace Tetrodotoxin::Library::Language::Access
