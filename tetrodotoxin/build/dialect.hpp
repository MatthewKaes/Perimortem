// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/environment/dialect.hpp"
#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Build {

// Build is the bootstrap language. Its inline Environment parser is private
// support and does not require another registered root Dialect.
class Dialect : public Tetrodotoxin::Language::Dialect {
 public:
  TTX_CONTRACT(Dialect, Tetrodotoxin::Language::Dialect);
  explicit Dialect(Perimortem::Core::View::Bytes name)
      : Tetrodotoxin::Language::Dialect(name),
        environment("Environment"_view) {}

  auto interpret(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Documentation& documentation,
      const Ttx::Lexical::Anchor& source_anchor,
      Ttx::Concept::Abstract& context)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Monograph&> override;

 private:
  Tetrodotoxin::Environment::Dialect environment;
};

}  // namespace Tetrodotoxin::Build
