// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/language/monograph.hpp"

namespace Tetrodotoxin::Render::Language {

// Monograph is the retained Render context. It begins with the borrowed outer
// Package surface and gains GPU Types and contracts only through Render owned
// declaration slices.
class Monograph : public Tetrodotoxin::Language::Monograph {
 public:
  TTX_CONTRACT(Monograph, Tetrodotoxin::Language::Monograph);

  static auto create(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& language,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context) -> Monograph&;

  TTX_NAME("Render"_view);

 private:
  Monograph(
      Perimortem::Memory::Allocator::Arena& arena,
      const Ttx::Concept::Abstract& language,
      const Ttx::Concept::Documentation& documentation,
      Ttx::Concept::Abstract& context)
      : Tetrodotoxin::Language::Monograph(
            arena,
            language,
            documentation,
            context) {}
};

}  // namespace Tetrodotoxin::Render::Language
