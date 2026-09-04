// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/abi.h"

namespace Ttx {

enum class LeafFit {
  Unknown,
  None,
  Accepted,
};

// A receiving Layout owns the question represented by requirement. This helper
// combines the total Domain edge with Interface negotiation without assuming
// that every requirement is a Domain or that matching native types prove fit.
auto fit_leaf(ttx_abstract producer, ttx_abstract requirement) -> LeafFit;

}  // namespace Ttx
