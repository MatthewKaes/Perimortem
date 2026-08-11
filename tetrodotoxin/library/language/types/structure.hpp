// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/core/option.hpp"

#include "perimortem/memory/allocator/arena.hpp"

#include "tetrodotoxin/language/definition.hpp"
#include "tetrodotoxin/library/language/types/composite.hpp"
#include "ttx/lexical/anchor.hpp"
#include "ttx/lexical/cursor.hpp"

namespace Tetrodotoxin::Library::Language::Types {

// Structure is one authored inline value Composite. Definition remains the
// single source for its name, Documentation, Attributes, modifiers, and kind.
class Structure : public Composite {
 protected:
  Structure(
      Perimortem::Memory::Allocator::Arena& domain,
      Tetrodotoxin::Language::Definition& definition,
      Monograph& source,
      Materializations& materializations,
      const Composite& enclosing_scope);

  static auto validate_definition(
      Ttx::Lexical::Cursor& cursor,
      const Tetrodotoxin::Language::Definition& definition) -> Bool;

  auto interpret_body(
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition,
      Ttx::Lexical::Token kind_token) -> Bool;

 public:
  TTX_CONTRACT(Structure, Composite, 0xe3773c0325224200, 0xaeb9a3131139c16f);

  static auto interpret(
      Perimortem::Memory::Allocator::Arena& domain,
      Ttx::Lexical::Cursor& cursor,
      Tetrodotoxin::Language::Definition& definition,
      Monograph& source,
      Materializations& materializations,
      const Composite& enclosing_scope) -> Perimortem::Core::Option<Structure&>;

  Structure(const Structure&) = delete;
  Structure(Structure&&) = delete;
  auto operator=(const Structure&) -> Structure& = delete;
  auto operator=(Structure&&) -> Structure& = delete;

  TTX_NAME(definition.get_name());

  TTX_DOCUMENTATION(definition.get_documentation());

  constexpr auto get_definition() const
      -> const Tetrodotoxin::Language::Definition& {
    return definition;
  }

  constexpr auto get_anchor() const
      -> Perimortem::Core::Option<Ttx::Lexical::Anchor> override {
    return anchor;
  }

 private:
  auto complete_definition(Ttx::Lexical::Anchor complete_anchor) -> void;

  Tetrodotoxin::Language::Definition& definition;
  Perimortem::Core::Option<Ttx::Lexical::Anchor> anchor;
};

}  // namespace Tetrodotoxin::Library::Language::Types
