// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/constants/flag.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Constants::Flag::lower(Llvm::Builder& body) const -> Bool {
  return body.flag_value(get_type(), *this, get_value());
}
