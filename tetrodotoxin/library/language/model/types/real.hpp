// # Tetrodotoxin
// Copyright (c) 2023-present Matt Kaes and contributors

#pragma once

#include "tetrodotoxin/library/language/model/types/value.hpp"

namespace Tetrodotoxin::Library::Language::Model::Types {

// Real proves only the exact Library numeric category. Representation and
// operation eligibility stay on the selected Type rather than a global table.
class Real : public Value {
 public:

  auto accepts_constant(const Ttx::Concept::Abstract&) const -> Bool override;
};

}  // namespace Tetrodotoxin::Library::Language::Model::Types
