// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/library/dialect.hpp"

namespace Tetrodotoxin::Package {

// Dialect parses one restricted Library export surface. Workspace owns the
// imported source graph, completion, and lifetime.
class Dialect : public Tetrodotoxin::Language::Dialect {
 public:
  TTX_CONTRACT(Dialect, Tetrodotoxin::Language::Dialect);

  Dialect(
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Library::Dialect& library)
      : Tetrodotoxin::Language::Dialect(name), library(library) {}

  Dialect(Tetrodotoxin::Library::Dialect& library)
      : Dialect("Package"_view, library) {}

  auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Monograph&> override;

  auto produce(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& graph,
      const Tetrodotoxin::Language::Monograph& monograph) const
      -> const ttx_pack* override;

  constexpr auto get_library() const -> Tetrodotoxin::Library::Dialect& {
    return library;
  }

 private:
  Tetrodotoxin::Library::Dialect& library;
};

}  // namespace Tetrodotoxin::Package
