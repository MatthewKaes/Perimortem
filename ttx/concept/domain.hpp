// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/concept/abstract.hpp"

namespace Ttx {

// A Pack can preserve its producer identity, but a receiving operation still
// needs to ask what kind of value that producer can supply. Domain answers that
// one question by returning the semantic owner of the value relationship and
// the Layout it currently projects. Languages remain free to build nominal,
// structural, recursive, or entirely different type systems above it because
// Domain defines no construction, storage, visibility, or lowering policy.
//
// Returning Layout beside the Domain identity also lets an owner synthesize
// partial shape for the current observation. A consumer receives that shape
// through the C ABI without acquiring the C++ object that produced it.
class Domain : public Abstract {
 public:
  using Abstract::Abstract;

  virtual auto layout() const -> ttx_layout = 0;
  void domain(ttx_abstract self, ttx_domain_result result) const final;
};

}  // namespace Ttx
