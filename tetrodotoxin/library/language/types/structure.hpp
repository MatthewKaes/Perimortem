// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/language/construction.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Structure is one authored inline value Composite. Definition remains the
// single source for its name, Documentation, Attributes, modifiers, and kind.
class Structure : public Composite {
 public:
  TTX_CONTRACT(Structure, Composite);

  static auto interpret(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition)
      -> Perimortem::Core::Option<Structure&>;

  static auto restore(
      Archive::Reader& reader,
      Perimortem::Memory::Allocator::Arena& arena,
      Ttx::Concept::Abstract& host,
      Tetrodotoxin::Language::Persistence::Profile profile)
      -> Perimortem::Core::Option<Structure&>;

  Structure(const Structure&) = delete;
  Structure(Structure&&) = delete;
  auto operator=(const Structure&) -> Structure& = delete;
  auto operator=(Structure&&) -> Structure& = delete;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

  auto link_fields(Ttx::Lexical::Cursor& cursor) -> Bool override;

  auto link_restored_fields() -> Bool override;

  auto reserve(Llvm::Program& program) const -> Bool override;

  auto complete(Llvm::Program& program) const -> Bool override;

  auto lower(Llvm::Program& program) const -> Bool override;

  auto resolve_type_call(
      const Ttx::Concept::Abstract& host,
      Perimortem::Core::View::Bytes route,
      Model::Type::Access access) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_construction() const
      -> Perimortem::Core::Option<const Construction&> {
    return construction
               ? Perimortem::Core::Option<const Construction&>(*construction)
               : Perimortem::Core::Option<const Construction&>();
  }

  auto persist(Archive::Writer& writer) const -> Bool override;

 protected:
  Structure(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Bool provider_construction = True)
      : Composite(domain, definition),
        provider_construction(provider_construction) {}

  auto interpret_body(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition,
      Ttx::Lexical::Token kind_token) -> Bool;

  auto reserve_carrier(Llvm::Program& program) const
      -> Perimortem::Core::Option<Bool> override;

  auto complete_carrier(Llvm::Program& program) const -> Bool override;

  auto complete_construction() -> Bool;

 private:
  Perimortem::Core::Option<Construction&> construction;
  Bool provider_construction;
  mutable Bool creating_default = False;
};

}  // namespace Tetrodotoxin::Library::Language::Types
