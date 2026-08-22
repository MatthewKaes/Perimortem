// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/definition.hpp"
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

  auto lower_provider(
      Llvm::Builder& body,
      const Model::Pack& result,
      const Model::Pack& arguments) const -> Bool override;

  auto reserve(Llvm::Program& program) const -> Bool override;

  auto complete(Llvm::Program& program) const -> Bool override;

  auto lower(Llvm::Program& program) const -> Bool override;

  auto persist(Archive::Writer& writer) const -> Bool override;

 protected:
  Structure(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Bool provides_initialization = True)
      : Composite(domain, definition),
        provides_initialization(provides_initialization) {}

  auto interpret_body(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition,
      Ttx::Lexical::Token kind_token) -> Bool;

  auto reserve_carrier(Llvm::Program& program) const
      -> Perimortem::Core::Option<Bool> override;

  auto complete_carrier(Llvm::Program& program) const -> Bool override;

  constexpr auto owns_initialization() const -> Bool {
    return provides_initialization;
  }

 private:
  Bool provides_initialization;
  mutable Bool creating_default = False;
};

}  // namespace Tetrodotoxin::Library::Language::Types
