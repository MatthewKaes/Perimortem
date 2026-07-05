// Perimortem Engine
// Copyright © Matt Kaes

#include "tetrodotoxin/isa/library/function.hpp"

#include "tetrodotoxin/isa/expression.hpp"
#include "tetrodotoxin/isa/layout/evaluator.hpp"

using namespace Perimortem::Core;
using namespace Perimortem::Memory;
using namespace Tetrodotoxin::Isa;
using namespace Ttx::Lexical;

auto Library::Function::evaluate(
    Scope& scope,
    Cursor& cursor,
    Ttx::Documentation documentation) -> Ttx::Type::Function {
  return evaluate(scope, cursor, documentation, BodyMode::Definition);
}

auto Library::Function::evaluate_declaration(
    Scope& scope,
    Cursor& cursor,
    Ttx::Documentation documentation) -> Ttx::Type::Function {
  return evaluate(scope, cursor, documentation, BodyMode::Declaration);
}

auto Library::Function::evaluate(
    Scope& scope,
    Cursor& cursor,
    Ttx::Documentation documentation,
    BodyMode body_mode) -> Ttx::Type::Function {
  if (!cursor.require(
          Class::Type::Func, "Expected `func` in library function."_view)) {
    return Ttx::Type::Function();
  }

  const Token* name = cursor.require(
      Class::Type::Addressable, "Expected library function name."_view);
  if (name == nullptr) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Member> parameters(
      scope.get_context().get_arena());
  if (!Layout::Evaluator::evaluate_bracketed(scope, cursor, parameters)) {
    return Ttx::Type::Function();
  }

  if (!cursor.require(
          Class::Type::CallOp,
          "Expected `->` before library function result."_view)) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Member> result(scope.get_context().get_arena());
  if (!Layout::Evaluator::evaluate(scope, cursor, result)) {
    return Ttx::Type::Function();
  }

  Managed::Vector<Ttx::Type::Function::Block> blocks(
      scope.get_context().get_arena());
  if (body_mode == BodyMode::Declaration) {
    if (!cursor.require(
            Class::Type::EndStatement,
            "Expected `;` after library function declaration."_view)) {
      return Ttx::Type::Function();
    }
  } else if (!Expression::consume_block(
                 cursor,
                 "Expected `{` after library function signature."_view,
                 "Expected `}` after library function body."_view, blocks)) {
    return Ttx::Type::Function();
  }

  return Ttx::Type::Function(
      name->get_text(), parameters.get_view(), result.get_view(),
      blocks.get_view(), documentation);
}
