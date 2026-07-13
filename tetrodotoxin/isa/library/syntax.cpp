// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/syntax.hpp"

#include "tetrodotoxin/isa/base/expression/evaluator.hpp"

using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Library::Syntax::consume_declaration_tail(
    Cursor& cursor,
    Bool consume_unmatched_scope) -> Bool {
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
        // Nested evaluators preserve their owner's closing brace. Root
        // evaluators have no owner and must consume it to guarantee progress.
        if (consume_unmatched_scope) {
          cursor.consume();
        }

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

    if (Base::Expression::Evaluator::is_index_start(
            cursor.current().get_class())) {
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
