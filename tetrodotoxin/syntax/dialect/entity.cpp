// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/syntax/dialect/entity.hpp"

using namespace Perimortem::Core;
using namespace Tetrodotoxin::Lexical;
using namespace Tetrodotoxin::Syntax;

auto Dialect::Entity::get_kind() const -> Class {
  return Class::Type::Entity;
}

auto Dialect::Entity::find_attribute(View::Bytes) const -> AttributeHandler {
  return AttributeHandler{};
}
