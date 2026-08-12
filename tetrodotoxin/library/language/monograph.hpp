// Perimortem Engine
// Copyright © Matt Kaes

#pragma once

#include "perimortem/memory/managed/vector.hpp"

#include "tetrodotoxin/library/dialect.hpp"
#include "tetrodotoxin/library/language/import.hpp"
#include "tetrodotoxin/library/language/materializations.hpp"
#include "tetrodotoxin/library/language/types/source.hpp"
#include "ttx/concept/reference.hpp"

namespace Tetrodotoxin::Library::Language {

// Monograph retains one Library source transaction. Its synthetic Source owns
// name lookup while Monograph keeps the source facts and ordered barriers that
// do not belong to a Type.
class Monograph : public Tetrodotoxin::Language::Monograph {
 private:
  Monograph(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      Tetrodotoxin::Library::Dialect& dialect,
      const Ttx::Concept::Abstract& interpretation_context,
      Materializations& materializations);

 public:
  TTX_CONTRACT(
      Monograph,
      Tetrodotoxin::Language::Monograph,
      0x4f5524bd23e84c30,
      0x8d7a8798bf1a25d2);

  static auto create_authored(
      Perimortem::Memory::Allocator::Arena& domain,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      Tetrodotoxin::Library::Dialect& dialect,
      const Ttx::Concept::Abstract& interpretation_context,
      Materializations& materializations) -> Monograph&;

  // Imports remain in authored order until linking can inspect each selected
  // Package member through its own contextual source route.
  auto retain_import(const Import& import) -> Bool;

  auto link() -> Bool override;

  auto finalize() -> Bool override;

  auto get_name() const -> Perimortem::Core::View::Bytes override;

  auto resolve_context(Perimortem::Core::View::Bytes route) const
      -> const Ttx::Concept::Abstract& override;

  constexpr auto get_source() -> Types::Source& { return source; }

  constexpr auto get_source() const -> const Types::Source& { return source; }

  constexpr auto get_materializations() -> Materializations& {
    return materializations;
  }

  constexpr auto get_dialect() const -> const Tetrodotoxin::Library::Dialect& {
    return dialect;
  }

  constexpr auto get_interpretation_context() const
      -> const Ttx::Concept::Abstract& {
    return interpretation_context;
  }

 private:
  auto link_imports() -> Bool;

  Tetrodotoxin::Library::Dialect& dialect;
  const Ttx::Concept::Abstract& interpretation_context;
  Materializations& materializations;
  Perimortem::Memory::Managed::Vector<Import> imports;
  Perimortem::Memory::Managed::Vector<Ttx::Concept::Reference<Monograph>>
      imported_providers;
  Types::Source& source;
  Bool declarations_imported = False;
  Bool addressables_imported = False;
};

}  // namespace Tetrodotoxin::Library::Language
