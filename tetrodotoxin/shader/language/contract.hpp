// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/shader/language/program.hpp"
#include "ttx/concept/interface.hpp"

namespace Tetrodotoxin::Shader::Language {

// Contract negotiates whether one real Shader Program can occupy the role of a
// Render Structure. Callable Layouts provide data flow evidence while Render
// Attributes and Shader binding relationships restore the policy that Layout
// intentionally omits. Neither side is copied into the other language.
class Contract : public Ttx::Concept::Interface {
 public:
  auto negotiate(
      const Ttx::Concept::Abstract& requirement,
      const Ttx::Concept::Abstract& candidate) const -> Relation override;

  auto validate(
      Ttx::Lexical::Cursor& cursor,
      const Ttx::Concept::Abstract& requirement,
      const Ttx::Concept::Abstract& candidate) const -> Bool;
};

}  // namespace Tetrodotoxin::Shader::Language
