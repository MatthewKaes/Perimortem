// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/abstract.hpp"

namespace Ttx {

// Domain connects one Abstract identity to the value shape it can currently
// describe. It is a graph relationship rather than a type hierarchy and adds
// no construction, storage, fitting, visibility, or lowering policy.
//
// The Layout remains an immutable projection for one observation. A derived
// owner may synthesize it on demand without exposing a native Domain object
// through the C ABI.
class Domain : public Abstract {
 public:
  using Abstract::Abstract;

  virtual auto layout() const -> ttx_layout = 0;
  void domain(ttx_abstract self, ttx_domain_result result) const final;
};

}  // namespace Ttx
