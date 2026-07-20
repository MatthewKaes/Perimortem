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
  while (!cursor.matches(Code::Type::Terminal)) {
    if (cursor.matches(Code::Type::ScopeStart)) {
      scope_depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Code::Type::ScopeEnd)) {
      if (scope_depth > 0) {
        scope_depth--;
        cursor.consume();
        continue;
      }

      break;
    }

    if (cursor.matches(Code::Type::PackingStart)) {
      packing_depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Code::Type::PackingEnd)) {
      if (packing_depth > 0) {
        packing_depth--;
        cursor.consume();
        continue;
      }

      break;
    }

    if (Base::Expression::Evaluator::is_index_start(
            cursor.current().get_code())) {
      index_depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Code::Type::LayoutEnd)) {
      if (index_depth > 0) {
        index_depth--;
        cursor.consume();
        continue;
      }

      break;
    }

    if (scope_depth == 0 && packing_depth == 0 && index_depth == 0 &&
        cursor.matches(Code::Type::EndStatement)) {
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
  if (!cursor.matches(Code::Type::Assign)) {
    return True;
  }

  cursor.consume();
  return consume(cursor, message);
}

auto Base::Expression::Evaluator::consume_block(
    Cursor& cursor,
    Perimortem::Core::View::Bytes open_error,
    Perimortem::Core::View::Bytes close_error) -> Bool {
  Bool has_scope = cursor.require(Code::Type::ScopeStart, open_error);
  if (!has_scope) {
    return False;
  }

  Count depth = 1;
  while (!cursor.matches(Code::Type::Terminal)) {
    if (cursor.matches(Code::Type::ScopeStart)) {
      depth++;
      cursor.consume();
      continue;
    }

    if (cursor.matches(Code::Type::ScopeEnd)) {
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
