// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/constants/real.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Constants::Real::lower(Llvm::Builder& body) const -> Bool {
  return body.real_value(get_type(), *this, get_value());
}
