// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/interface.hpp"

namespace Ttx::Model::Interfaces {

// Callable negotiates whether one real Callable can stand in for another.
// Parameters agree in both directions so the candidate accepts the promised
// inputs. Candidate results flow toward the required result Layout, while the
// reverse result fit distinguishes complete equivalence from satisfaction.
class Callable : public Concept::Interface {
 public:
  auto negotiate(
      const Concept::Abstract& requirement,
      const Concept::Abstract& candidate) const -> Relation override;
};

}  // namespace Ttx::Model::Interfaces
