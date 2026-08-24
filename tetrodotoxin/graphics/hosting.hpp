// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/interface.hpp"

namespace Tetrodotoxin::Graphics {

// Hosting proves that one real Library Object can occupy an authored graphics
// requirement. The requirement remains the semantic source of its public state
// while the candidate keeps its own identity and any richer domain behavior.
// Runtime descriptors are derived only after this relation succeeds.
class Hosting : public Ttx::Concept::Interface {
 public:
  auto negotiate(
      const Ttx::Concept::Abstract& requirement,
      const Ttx::Concept::Abstract& candidate) const -> Relation override;
};

}  // namespace Tetrodotoxin::Graphics
