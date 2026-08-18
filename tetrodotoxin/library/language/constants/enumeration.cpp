// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/constants/enumeration.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Constants::Enumeration::lower(Llvm::Builder& body) const -> Bool {
  return body.enumeration_value(get_type(), *this, get_value());
}
