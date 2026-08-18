// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/library/language/constants/bytes.hpp"

#include "tetrodotoxin/library/llvm/builder.hpp"

using namespace Tetrodotoxin::Library;

auto Language::Constants::Bytes::lower(Llvm::Builder& body) const -> Bool {
  return body.bytes_value(get_type(), *this, get_value());
}
