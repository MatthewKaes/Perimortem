// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.hpp"

namespace Ttx {

// Constant lets a consumer retain one completed immutable answer as evidence
// without freezing the live route which produced it. Folding, hashing, and
// packaging remain separate contracts that a language may ask this completed
// answer to join rather than operations inherited by the host-neutral category.
class Constant : public Abstract {
 public:
  using Abstract::Abstract;

 protected:
  auto negotiate(ttx_abstract requirement) const
      -> ttx_interface_relation override;
};

}  // namespace Ttx

namespace Ttx::Concept {

using Constant = Ttx::Constant;

}  // namespace Ttx::Concept
