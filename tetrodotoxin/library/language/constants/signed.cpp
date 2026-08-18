// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/constants/signed.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Constants::Signed::lower(Llvm::Builder& body) const -> Bool {
  return body.signed_value(get_type(), *this, get_value());
}
