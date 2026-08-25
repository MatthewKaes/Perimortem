// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/dialect.hpp"
#include "tetrodotoxin/library/dialect.hpp"

namespace Tetrodotoxin::Scene {

// Scene owns its outer source policy while borrowing the exact installed
// Library Dialect that constructs the one semantic child beneath it.
class Dialect : public Tetrodotoxin::Language::Dialect {
 public:
  TTX_CONTRACT(Dialect, Tetrodotoxin::Language::Dialect);

  Dialect(
      Perimortem::Core::View::Bytes name,
      Tetrodotoxin::Library::Dialect& library)
      : Tetrodotoxin::Language::Dialect(name), library(library) {}

  Dialect(Tetrodotoxin::Library::Dialect& library)
      : Dialect("Scene"_view, library) {}

  auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Monograph&> override;

  auto encode(
      const Ttx::Concept::Abstract& monograph,
      Tetrodotoxin::Language::Persistence::Profile profile) const
      -> Perimortem::Core::Option<Perimortem::Memory::Dynamic::Bytes> override;

  auto restore(
      Perimortem::Memory::Allocator::Arena& arena,
      Perimortem::Core::View::Bytes payload,
      Tetrodotoxin::Language::Persistence::Profile profile,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Monograph&> override;

 private:
  Tetrodotoxin::Library::Dialect& library;
};

}  // namespace Tetrodotoxin::Scene
