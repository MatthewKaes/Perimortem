// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/function.hpp"

#include "tetrodotoxin/isa/base/layout/evaluator.hpp"
#include "tetrodotoxin/isa/library/syntax.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Library::Function::evaluate(
    Cursor& cursor,
    Scope& scope,
    Ttx::Documentation documentation,
    Perimortem::Utility::Range& source) -> Ttx::Function {
  return evaluate(cursor, scope, documentation, BodyMode::Definition, source);
}

auto Library::Function::evaluate_declaration(
    Cursor& cursor,
    Scope& scope,
    Ttx::Documentation documentation) -> Ttx::Function {
  Perimortem::Utility::Range source;
  return evaluate(cursor, scope, documentation, BodyMode::Declaration, source);
}

auto Library::Function::evaluate(
    Cursor& cursor,
    Scope& scope,
    Ttx::Documentation documentation,
    BodyMode body_mode,
    Perimortem::Utility::Range& source) -> Ttx::Function {
  if (!cursor.require(
          Class::Type::Func, "Expected `func` in library function."_view)) {
    return Ttx::Function();
  }

  const Token* name = cursor.require(
      Class::Type::Addressable, "Expected library function name."_view);
  if (name == nullptr) {
    return Ttx::Function();
  }

  Managed::Vector<Ttx::Member> parameters(scope.get_context().get_arena());
  if (!Base::Layout::Evaluator::evaluate_bracketed(cursor, scope, parameters)) {
    return Ttx::Function();
  }

  if (!cursor.require(
          Class::Type::CallOp,
          "Expected `->` before library function result."_view)) {
    return Ttx::Function();
  }

  Managed::Vector<Ttx::Member> result(scope.get_context().get_arena());
  if (!Base::Layout::Evaluator::evaluate(cursor, scope, result)) {
    return Ttx::Function();
  }

  Ttx::Function function(
      name->get_text(), Ttx::Layout(parameters.get_view()),
      Ttx::Layout(result.get_view()), documentation);
  if (body_mode == BodyMode::Declaration) {
    if (!cursor.require(
            Class::Type::EndStatement,
            "Expected `;` after library function declaration."_view)) {
      return Ttx::Function();
    }
  } else if (cursor.matches(Class::Type::EndStatement)) {
    cursor.consume();
  } else {
    Count start = cursor.get_token_index();
    if (!cursor.matches(Class::Type::ScopeStart) ||
        !Syntax::consume_declaration_tail(cursor)) {
      return Ttx::Function();
    }

    Count end = cursor.get_token_index();
    View::Vector<Token> tail = cursor.get_token_span(end - 1, end);
    if (tail.is_empty() || tail[0].get_class() != Class::Type::ScopeEnd) {
      cursor.token_error("Expected `}` after library function body."_view);
      return Ttx::Function();
    }

    source = {start, end - start};
  }

  return function;
}
