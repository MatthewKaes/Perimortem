// Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "ttx/model/layouts/ranged.hpp"

namespace Ttx::Model::Layouts {

// Value is the terminal one entry descriptor. Atomic Types retain their exact
// identity here while structural Types replace the leaf with their real Layout.
// Specializing Ranged keeps fitting and fitted source behavior aligned with
// every other positional Layout without storing a second edge.
class Value : public Ranged {
 public:
  constexpr explicit Value(const Concept::Abstract& type) : Ranged(type, 1) {}
};

}  // namespace Ttx::Model::Layouts
