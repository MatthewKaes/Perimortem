// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/attribute.hpp"

using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Attribute::consume_all(Cursor& cursor) -> Bool {
  while (cursor.matches(Class::Type::Attribute)) {
    if (!consume(cursor)) {
      return False;
    }
  }

  return True;
}

auto Attribute::consume(Cursor& cursor) -> Bool {
  if (!cursor.require(Class::Type::Attribute, "Expected attribute."_view)) {
    return False;
  }

  if (!cursor.matches(Class::Type::PackingStart)) {
    return True;
  }

  Count depth = 0;
  while (!cursor.matches(Class::Type::EndOfStream)) {
    if (cursor.matches(Class::Type::PackingStart)) {
      depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::PackingEnd)) {
      cursor.consume();
      depth--;
      if (depth == 0) {
        return True;
      }
      continue;
    }

    cursor.consume();
  }

  cursor.token_error("Expected `)` after attribute."_view);
  return False;
}
