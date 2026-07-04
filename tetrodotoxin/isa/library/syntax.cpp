// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/syntax.hpp"

using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Library::Syntax::consume_declaration_tail(Cursor& cursor) -> Bool {
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
      if (scope_depth == 0) {
        return True;
      }

      cursor.consume();
      scope_depth--;
      if (scope_depth == 0) {
        return True;
      }
      continue;
    }

    if (cursor.matches(Class::Type::PackingStart)) {
      packing_depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::PackingEnd)) {
      if (packing_depth > 0) {
        packing_depth--;
      }
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::IndexStart) ||
        cursor.matches(Class::Type::SliceOp) ||
        cursor.matches(Class::Type::SwizzleOp)) {
      index_depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::IndexEnd)) {
      if (index_depth > 0) {
        index_depth--;
      }
      cursor.consume();
      continue;
    }

    if (scope_depth == 0 && packing_depth == 0 && index_depth == 0 &&
        cursor.matches(Class::Type::EndStatement)) {
      cursor.consume();
      return True;
    }

    cursor.consume();
  }

  return True;
}

auto Library::Syntax::consume_initializer(
    Cursor& cursor,
    Perimortem::Core::View::Bytes error_message) -> Bool {
  if (!cursor.matches(Class::Type::Assign)) {
    return True;
  }

  cursor.consume();
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
      }
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::PackingStart)) {
      packing_depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::PackingEnd)) {
      if (packing_depth > 0) {
        packing_depth--;
      }
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::IndexStart) ||
        cursor.matches(Class::Type::SliceOp) ||
        cursor.matches(Class::Type::SwizzleOp)) {
      index_depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::IndexEnd)) {
      if (index_depth > 0) {
        index_depth--;
      }
      cursor.consume();
      continue;
    }

    if (scope_depth == 0 && packing_depth == 0 && index_depth == 0 &&
        cursor.matches(Class::Type::EndStatement)) {
      return True;
    }

    cursor.consume();
  }

  cursor.token_error(error_message);
  return False;
}
