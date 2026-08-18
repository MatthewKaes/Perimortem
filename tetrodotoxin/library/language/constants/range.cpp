// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/constants/range.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Constants::Range::lower(Llvm::Builder& body) const -> Bool {
  return body.empty_range(get_type(), *this);
}
