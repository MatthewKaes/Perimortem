// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/base/expression/evaluator.hpp"

using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Base::Expression::Evaluator::consume(
    Cursor& cursor,
    Perimortem::Core::View::Bytes message) -> Bool {
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

    if (Base::Expression::Evaluator::is_index_start(
            cursor.current().get_class())) {
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

auto Base::Expression::Evaluator::consume_initializer(
    Cursor& cursor,
    Perimortem::Core::View::Bytes message) -> Bool {
  if (!cursor.matches(Class::Type::Assign)) {
    return True;
  }

  cursor.consume();
  return consume(cursor, message);
}

auto Base::Expression::Evaluator::consume_block(
    Cursor& cursor,
    Perimortem::Core::View::Bytes open_error,
    Perimortem::Core::View::Bytes close_error) -> Bool {
  if (!cursor.require(Class::Type::ScopeStart, open_error)) {
    return False;
  }

  Count depth = 1;
  while (!cursor.matches(Class::Type::EndOfStream)) {
    if (cursor.matches(Class::Type::ScopeStart)) {
      depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Class::Type::ScopeEnd)) {
      if (depth == 1) {
        cursor.consume();
        return True;
      }

      depth--;
      cursor.consume();
      continue;
    }

    cursor.consume();
  }

  cursor.token_error(close_error);
  return False;
}
