// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/language/model/admission.hpp"
#include "tetrodotoxin/library/language/model/initialization.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Structure is one authored inline value Composite. Definition remains the
// single source for its name, Documentation, Attributes, modifiers, and kind.
class Structure : public Composite {
 public:

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition) -> Structure&;

  static auto create_restored(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition) -> Structure&;

  Structure(const Structure&) = delete;
  Structure(Structure&&) = delete;
  auto operator=(const Structure&) -> Structure& = delete;
  auto operator=(Structure&&) -> Structure& = delete;

  auto resolve_concept(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;
  void visit_concepts(ttx_named_abstract_callable* visitor) const override;

  virtual auto initialize_default(Perimortem::Memory::Allocator::Arena& arena)
      const -> Perimortem::Core::Option<Model::Pack&>;

  virtual auto initialize_supplied(
      Ttx::Lexical::Cursor& cursor,
      Model::Pack& source,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope,
      Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor) const
      -> Perimortem::Core::Option<Model::Pack&>;

  virtual auto initialize_supplied_restored(
      Perimortem::Memory::Allocator::Arena& arena,
      Model::Pack& source,
      Perimortem::Core::Option<const Ttx::Concept::Abstract&> access_scope)
      const -> Perimortem::Core::Option<Model::Pack&>;

  auto accepts(const Model::Pack& source) const -> Bool;

  auto create_admitted(
      Perimortem::Memory::Allocator::Arena& arena,
      Model::Pack& source) const -> Perimortem::Core::Option<Model::Pack&>;

  constexpr auto has_initialization_provider() const -> Bool {
    return provides_initialization;
  }

  // The closing brace fixes member identity and source order even though
  // individual Type edges settle later. Source interpretation calls this once
  // after the complete authored body has been retained.
  auto complete_body() -> void override;

 protected:
  Structure(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Bool provides_initialization = True)
      : Composite(domain, definition),
        provides_initialization(provides_initialization),
        admission(*this),
        initialization(*this) {}

  constexpr auto owns_initialization() const -> Bool {
    return provides_initialization;
  }

  virtual constexpr auto supports_initialization() const -> Bool {
    return True;
  }

 private:
  Bool provides_initialization;
  Model::OwnedAdmission<Structure> admission;
  Model::OwnedInitialization<Structure> initialization;
  mutable Bool creating_default = False;
};

}  // namespace Tetrodotoxin::Library::Language::Types
