// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/expression.hpp"

using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Expression::consume(Cursor& cursor, Perimortem::Core::View::Bytes message)
    -> Bool {
  Count scope_depth = 0;
  Count packing_depth = 0;
  Count index_depth = 0;
  while (!cursor.matches(Class::Type::EndOfStream)) {
    if (cursor.matches(Class::Type::ScopeStart)) {
      scope_depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::ScopeEnd)) {
      if (scope_depth > 0) {
        scope_depth--;
        cursor.consume();
        continue;
      }
      break;
    }

    if (cursor.matches(Class::Type::PackingStart)) {
      packing_depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::PackingEnd)) {
      if (packing_depth > 0) {
        packing_depth--;
        cursor.consume();
        continue;
      }
      break;
    }

    if (Expression::is_index_start(cursor.current().get_class())) {
      index_depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::IndexEnd)) {
      if (index_depth > 0) {
        index_depth--;
        cursor.consume();
        continue;
      }
      break;
    }

    if (scope_depth == 0 && packing_depth == 0 && index_depth == 0 &&
        cursor.matches(Class::Type::EndStatement)) {
      return True;
    }

    cursor.consume();
  }

  cursor.token_error(message);
  return False;
}

auto Expression::consume_initializer(
    Cursor& cursor,
    Perimortem::Core::View::Bytes message) -> Bool {
  if (!cursor.matches(Class::Type::Assign)) {
    return True;
  }

  cursor.consume();
  return consume(cursor, message);
}
