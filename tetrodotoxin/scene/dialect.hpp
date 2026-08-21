// Perimortem Engine
// Copyright © Matt Kaes

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

 private:
  Tetrodotoxin::Library::Dialect& library;
};

}  // namespace Tetrodotoxin::Scene
