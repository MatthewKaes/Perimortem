// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"
#include "perimortem/memory/managed/map.hpp"

#include "ttx/concept/none.hpp"
#include "ttx/concept/reference.hpp"
#include "ttx/concept/unknown.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Instance is one Composite's value-level authority. State Fields and Self
// Callables share one spelling space and retain their original identities.
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
  auto get_concepts(Ttx::Concept::Context& context) const
      -> const Ttx::Concept::Pack& override;

  constexpr auto complete() -> void { completed = True; }

 private:
  class Binding {
   public:
    constexpr Binding(Ttx::Concept::Abstract& semantic, Bool published)
        : semantic(semantic), published(published) {}

    Ttx::Concept::Reference<Ttx::Concept::Abstract> semantic;
    Bool published;
  };

  class Surface : public Ttx::Concept::Layout {
   public:
    constexpr explicit Surface(const Instance& owner) : owner(owner) {}

    auto get_size() const -> Count override;
    auto get_abstract(Count index) const
        -> Perimortem::Core::Option<const Ttx::Concept::Abstract&> override;
    auto get_name(Count index) const
        -> Perimortem::Core::Option<Perimortem::Core::View::Bytes> override;
    auto fits_entry(
        const Ttx::Concept::Layout& target,
        Count source_index,
        Count target_index) const -> Bool override;
    auto fits_at(const Ttx::Concept::Layout& target, Count target_offset) const
        -> Bool override;
    auto get_fitted_at(
        const Ttx::Concept::Layout& target,
        Count target_offset,
        Count target_index) const -> Perimortem::Utility::
        Result<const Ttx::Concept::Abstract&, Errors> override;

   private:
    auto select(Count index) const -> const Binding*;

    const Instance& owner;
  };

  Perimortem::Memory::Managed::Map<Perimortem::Core::View::Bytes, Binding>
      bindings;
  Bool completed = False;
};

}  // namespace Tetrodotoxin::Library::Language::Types
