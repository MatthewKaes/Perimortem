// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/constants/unsigned.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Constants::Unsigned::lower(Llvm::Builder& body) const -> Bool {
  return body.unsigned_value(get_type(), *this, get_value());
}
