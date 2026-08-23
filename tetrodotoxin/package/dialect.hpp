// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Package {

// Dialect parses one Package description table and constructs its semantic
// root. Workspace owns source acquisition, completion, and lifetime.
class Dialect : public Tetrodotoxin::Language::Dialect {
 public:
  TTX_CONTRACT(Dialect, Tetrodotoxin::Language::Dialect);

  Dialect(Perimortem::Core::View::Bytes name = "Package"_view)
      : Tetrodotoxin::Language::Dialect(name) {}

  auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<
          Tetrodotoxin::Language::Monograph&> override;
};

}  // namespace Tetrodotoxin::Package
