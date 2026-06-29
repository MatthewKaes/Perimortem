// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/dialect/library.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Lexical;
using namespace Tetrodotoxin::Syntax;

auto Dialect::Library::get_kind() const -> Class {
  return Class::Type::Library;
}

auto Dialect::Library::find_attribute(View::Bytes) const -> AttributeHandler {
  return AttributeHandler{};
}
