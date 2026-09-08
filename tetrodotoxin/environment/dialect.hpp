// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once
#include "tetrodotoxin/environment/plan.hpp"
#include "tetrodotoxin/language/dialect.hpp"

namespace Tetrodotoxin::Environment {
// Build delegates its inline host description to this installed Dialect.
// The Package can then be compiled without embedding host locators in its
// graph.
class Dialect : public Tetrodotoxin::Language::Dialect {
 public:
  TTX_CONTRACT(Dialect, Tetrodotoxin::Language::Dialect);
  explicit Dialect(Perimortem::Core::View::Bytes name)
      : Language::Dialect(name) {}
  auto interpret(
      Ttx::Lexical::Cursor&,
      const Ttx::Concept::Documentation&,
      const Ttx::Lexical::Anchor&,
      Ttx::Concept::Abstract&)
      -> Perimortem::Core::Option<Tetrodotoxin::Language::Monograph&> override;
  auto parse_region(Ttx::Lexical::Cursor&) const
      -> Perimortem::Core::Option<Plan&>;
};
}  // namespace Tetrodotoxin::Environment
