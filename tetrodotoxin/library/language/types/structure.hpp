// Perimortem Engine
// Copyright © Matt Kaes

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

  Structure(const Structure&) = delete;
  Structure(Structure&&) = delete;
  auto operator=(const Structure&) -> Structure& = delete;
  auto operator=(Structure&&) -> Structure& = delete;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  auto create_default(Perimortem::Memory::Allocator::Arena& arena) const
      -> Perimortem::Core::Option<Model::Pack&> override;

 protected:
  Structure(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition)
      : Composite(domain, definition) {}

  auto interpret_body(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition,
      Ttx::Lexical::Token kind_token) -> Bool;

  auto reserve_carrier(Llvm::Program& program) const
      -> Perimortem::Core::Option<Bool> override;

  auto complete_carrier(Llvm::Program& program) const -> Bool override;
};

}  // namespace Tetrodotoxin::Library::Language::Types
