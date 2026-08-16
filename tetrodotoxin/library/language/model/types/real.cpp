// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/model/types/real.hpp"

#include "tetrodotoxin/library/language/constants/real.hpp"

using namespace Tetrodotoxin::Library::Language;
using namespace Ttx::Concept;

auto Model::Types::Real::accepts_constant(const Abstract& value) const -> Bool {
  return value.visit<Constants::Real>(
      [this](const Constants::Real& selected) -> Bool {
        return &selected.get_type().resolve() == &resolve() ? True : False;
      },
      [](const Abstract&) -> Bool { return False; });
}
